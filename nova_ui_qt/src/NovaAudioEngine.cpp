#include "NovaAudioEngine.h"
#include "core/NovaLogging.h"
#include "core/NovaAudioDeviceManager.h"
#include "core/NovaHardwareSanitizer.h"
#include "core/NovaProjectManager.h"
#include "models/NovaTrackListModel.h"
#include "models/NovaRegionModel.h"
#include "views/NovaWaveformItem.h"
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <cmath>
#include <algorithm>

#if !defined(Q_OS_ANDROID)
#pragma push_macro("emit")
#pragma push_macro("slots")
#pragma push_macro("signals")
#pragma push_macro("foreach")
#undef emit
#undef slots
#undef signals
#undef foreach

#include "ardour/session.h"
#include "ardour/route.h"
#include "ardour/meter.h"
#include "ardour/gain_control.h"

#pragma pop_macro("emit")
#pragma pop_macro("slots")
#pragma pop_macro("signals")
#pragma pop_macro("foreach")
#endif

struct _VSTState;
int vstfx_init(void*) { return 0; }
void vstfx_exit() {}
void vstfx_destroy_editor(_VSTState*) {}

NovaAudioEngine::NovaAudioEngine(QObject *parent)
    : QObject(parent)
{
    m_trackModel = new NovaTrackListModel(this);
    m_regionModel = new NovaRegionModel(this);

    // Conexión del transporte a la UI (60 FPS tick)
    connect(&m_transport, &NovaTransportController::isPlayingChanged, this, &NovaAudioEngine::isPlayingChanged);
    connect(&m_transport, &NovaTransportController::timecodeChanged, this, &NovaAudioEngine::timecodeChanged);
    connect(&m_transport, &NovaTransportController::bbtChanged, this, &NovaAudioEngine::bbtChanged);
    connect(&m_transport, &NovaTransportController::positionChanged, this, &NovaAudioEngine::positionChanged);
    connect(&m_transport, &NovaTransportController::positionChanged, this, &NovaAudioEngine::masterPeaksChanged); 
    connect(&m_transport, &NovaTransportController::bpmChanged, this, &NovaAudioEngine::bpmChanged);
    connect(&m_transport, &NovaTransportController::loopEnabledChanged, this, &NovaAudioEngine::loopEnabledChanged);
    connect(&m_transport, &NovaTransportController::loopRangeChanged, this, &NovaAudioEngine::loopRangeChanged);

    // Conectar el grabador de pistas
    connect(&m_recorder, &NovaRecordManager::isRecordingChanged, this, &NovaAudioEngine::isRecordingChanged);
}

NovaAudioEngine::~NovaAudioEngine()
{
    qCDebug(novaCore) << "Destruyendo NovaAudioEngine... Cierre seguro de recursos.";
    if (m_regionModel) m_regionModel->waitForImports();
    m_transport.setSession(nullptr);
}

bool NovaAudioEngine::initEngine()
{
    // 🛡️ Sanitización preventiva de hardware al arrancar
    NovaHardwareSanitizer::sanitize();

    // Inicializar el subsistema del administrador de proyectos
    bool ok = m_sessionManager.initSession();
    if (!ok) {
        qCCritical(novaCore) << "Fallo fatal al arrancar m_sessionManager.";
        return false;
    }

    connectSessionSignals();
    return true;
}

void NovaAudioEngine::connectSessionSignals()
{
    auto session = m_sessionManager.session();
    m_transport.setSession(session);
    m_recorder.setSession(session);
    m_recorder.setRegionModel(m_regionModel);
    m_recorder.setTransportController(&m_transport);
    m_trackModel->setSession(session);
    m_regionModel->setSession(session);
    m_deviceManager.setEngine(m_sessionManager.engine());

    // Conectar el renderizador global de formas de onda
    NovaWaveformItem::setSession(session);

    // Notificar cambios del proyecto a la interfaz
    Q_EMIT currentProjectChanged();
    Q_EMIT isDirtyChanged();
    Q_EMIT tracksChanged();
    Q_EMIT regionsChanged();
    Q_EMIT recentProjectsChanged();
}

// Controles de Transporte
void NovaAudioEngine::play() { m_transport.play(); }
void NovaAudioEngine::pause() { m_transport.stop(); }
void NovaAudioEngine::stop() { m_transport.stop(); }
void NovaAudioEngine::togglePlay() { m_transport.togglePlay(); }
void NovaAudioEngine::toggleRecord() { m_recorder.toggleRecord(); }
void NovaAudioEngine::locateFrame(double frame) { m_transport.locateFrame(frame); }
void NovaAudioEngine::locateBeat(double beat) { m_transport.locateBeat(beat); }
void NovaAudioEngine::setBpm(double bpm) { m_transport.setBpm(bpm); }

void NovaAudioEngine::setLoopEnabled(bool enabled) { m_transport.setLoopEnabled(enabled); }
void NovaAudioEngine::setLoopRange(double startBeat, double endBeat) { m_transport.setLoopRange(startBeat, endBeat); }

// 💾 GUARDAR EL PROYECTO ACTUAL (DESDE BOTÓN O CTRL + S)
bool NovaAudioEngine::saveProject()
{
    bool ok = NovaProjectManager::saveProject(m_sessionManager.session());
    if (ok) {
        Q_EMIT isDirtyChanged();
        Q_EMIT recentProjectsChanged();
    }
    return ok;
}

// 📂 CARGAR UN PROYECTO EXISTENTE DESDE DISCO
bool NovaAudioEngine::openProject(const QString &path)
{
    qCDebug(novaCore) << "🔊 Solicitada apertura del proyecto en:" << path;
    m_sessionManager.closeCurrentSession();
    
    auto session = NovaProjectManager::openProject(m_sessionManager.engine(), path);
    m_sessionManager.setSession(session);
    
    if (session) {
        connectSessionSignals();
        return true;
    }
    return false;
}

// 🆕 CREAR UN NUEVO PROYECTO LIMPIO INDEPENDIENTE
bool NovaAudioEngine::newProject(const QString &name, const QString &parentDir)
{
    qCDebug(novaCore) << "🔊 Creando nuevo proyecto comercial:" << name << "en:" << parentDir;
    m_sessionManager.closeCurrentSession();
    
    auto session = NovaProjectManager::createProject(m_sessionManager.engine(), name, parentDir);
    m_sessionManager.setSession(session);
    
    if (session) {
        connectSessionSignals();
        return true;
    }
    return false;
}

// 🚪 CERRAR EL PROYECTO ACTIVO Y LIBERAR RECURSOS
void NovaAudioEngine::closeProject()
{
    qCDebug(novaCore) << "🚪 Liberando proyecto y retornando a la Start Screen...";
    m_transport.setSession(nullptr);
    m_recorder.setSession(nullptr);
    m_trackModel->setSession(nullptr);
    m_regionModel->setSession(nullptr);
    m_sessionManager.closeCurrentSession();
    
    Q_EMIT currentProjectChanged();
    Q_EMIT isDirtyChanged();
    Q_EMIT tracksChanged();
    Q_EMIT regionsChanged();
    Q_EMIT recentProjectsChanged();
}

// 🗑️ DESCARTAR CAMBIOS Y ELIMINAR PROYECTO NO GUARDADO
void NovaAudioEngine::discardProject()
{
    qCDebug(novaCore) << "🗑️ Descartando proyecto activo sin guardar...";
    if (m_sessionManager.session()) {
        NovaProjectManager::discardUnsavedProject(m_sessionManager.session());
    }
    closeProject();
}

// 🗑️ ELIMINAR PROYECTO DEL HISTORIAL DE RECIENTES
void NovaAudioEngine::removeRecentProject(const QString &path)
{
    NovaProjectManager::removeProjectFromRecent(path);
    Q_EMIT recentProjectsChanged();
}

bool NovaAudioEngine::isDirty() const
{
    return NovaProjectManager::isDirty(m_sessionManager.session());
}

QVariantList NovaAudioEngine::recentProjects() const
{
    return NovaProjectManager::getRecentProjects();
}

QString NovaAudioEngine::currentProjectName() const
{
    auto session = m_sessionManager.session();
    if (!session) return QStringLiteral("No Project");
    return QString::fromStdString(session->name());
}

QString NovaAudioEngine::currentProjectPath() const
{
    auto session = m_sessionManager.session();
    if (!session) return QString();
    return QString::fromStdString(session->path());
}

float NovaAudioEngine::masterPeakLeft() const
{
#if defined(Q_OS_ANDROID)
    return 0.0f;
#else
    auto session = m_sessionManager.session();
    if (!session) return 0.0f;

    auto master = session->master_out();
    if (!master) return 0.0f;

    auto meter = master->peak_meter();
    if (!meter) return 0.0f;

    float val = meter->meter_level(0, ARDOUR::MeterPeak);
    if (std::isnan(val) || std::isinf(val) || val < 0.0f) return 0.0f;
    return val;
#endif
}

float NovaAudioEngine::masterPeakRight() const
{
#if defined(Q_OS_ANDROID)
    return 0.0f;
#else
    auto session = m_sessionManager.session();
    if (!session) return 0.0f;

    auto master = session->master_out();
    if (!master) return 0.0f;

    auto meter = master->peak_meter();
    if (!meter) return 0.0f;

    float val = meter->meter_level(1, ARDOUR::MeterPeak);
    if (std::isnan(val) || std::isinf(val) || val < 0.0f) return 0.0f;
    return val;
#endif
}

float NovaAudioEngine::masterVolumeDb() const
{
#if defined(Q_OS_ANDROID)
    return 0.0f;
#else
    auto session = m_sessionManager.session();
    if (!session) return 0.0f;

    auto master = session->master_out();
    if (!master) return 0.0f;

    auto gc = master->gain_control();
    if (!gc) return 0.0f;

    float coeff = static_cast<float>(gc->get_value());
    if (coeff <= 0.0000001f) return -192.0f;
    return 20.0f * std::log10(coeff);
#endif
}

void NovaAudioEngine::setMasterVolumeDb(float dB)
{
#if !defined(Q_OS_ANDROID)
    auto session = m_sessionManager.session();
    if (!session) return;

    auto master = session->master_out();
    if (!master) return;

    auto gc = master->gain_control();
    if (gc) {
        float coeff = (dB <= -192.0f) ? 0.0f : std::pow(10.0f, dB / 20.0f);
        gc->set_value(coeff, PBD::Controllable::NoGroup);
        Q_EMIT masterVolumeDbChanged();
    }
#else
    Q_UNUSED(dB);
#endif
}