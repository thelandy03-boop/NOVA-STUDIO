#include "NovaAudioEngine.h"
#include "core/NovaLogging.h"
#include "core/NovaAudioDeviceManager.h"
#include "core/NovaHardwareSanitizer.h"
#include "models/NovaTrackListModel.h"
#include "models/NovaRegionModel.h"
#include "views/NovaWaveformItem.h"
#include <QDebug>
#include <cmath>
#include <algorithm>

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

// ── STUBS GLOBALES VST PARA RESOLVER SÍMBOLOS DE LINKER EN LIBARDOUR.SO ──
struct _VSTState;
int vstfx_init(void*) { return 0; }
void vstfx_exit() {}
void vstfx_destroy_editor(_VSTState*) {}

NovaAudioEngine::NovaAudioEngine(QObject *parent)
    : QObject(parent)
{
    m_trackModel = new NovaTrackListModel(this);
    m_regionModel = new NovaRegionModel(this);

    // Conectar señales del transporte
    connect(&m_transport, &NovaTransportController::isPlayingChanged, this, &NovaAudioEngine::isPlayingChanged);
    connect(&m_transport, &NovaTransportController::timecodeChanged, this, &NovaAudioEngine::timecodeChanged);
    connect(&m_transport, &NovaTransportController::bbtChanged, this, &NovaAudioEngine::bbtChanged);
    connect(&m_transport, &NovaTransportController::positionChanged, this, &NovaAudioEngine::positionChanged);
    connect(&m_transport, &NovaTransportController::positionChanged, this, &NovaAudioEngine::masterPeaksChanged); // 60 FPS tick de vúmetro
    connect(&m_transport, &NovaTransportController::bpmChanged, this, &NovaAudioEngine::bpmChanged);
    connect(&m_transport, &NovaTransportController::loopEnabledChanged, this, &NovaAudioEngine::loopEnabledChanged);
    connect(&m_transport, &NovaTransportController::loopRangeChanged, this, &NovaAudioEngine::loopRangeChanged);

    // Conectar señales del grabador
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
    // 🛡️ 1. Sanitización de Hardware ALSA en segundo plano (Estilo BandLab/FL Studio)
    NovaHardwareSanitizer::sanitize();

    // 🚀 2. Inicializar sesión y subsistemas de Ardour Core
    bool ok = m_sessionManager.initSession();
    if (!ok) {
        qCCritical(novaCore) << "Fallo al inicializar la sesión de audio.";
        return false;
    }

    auto session = m_sessionManager.session();
    if (session) {
        m_transport.setSession(session);
        m_recorder.setSession(session);
        m_recorder.setRegionModel(m_regionModel);
        m_recorder.setTransportController(&m_transport);
        m_trackModel->setSession(session);
        m_regionModel->setSession(session);
        m_deviceManager.setEngine(m_sessionManager.engine());

        // 🚀 Sincronizar el renderizador de formas de onda con la sesión activa
        NovaWaveformItem::setSession(session);

        qCDebug(novaCore) << "🚀 Arquitectura Modular Inicializada con Éxito con Motor Real.";
        return true;
    }

    return false;
}

float NovaAudioEngine::masterPeakLeft() const
{
    auto session = m_sessionManager.session();
    if (!session) return 0.0f;

    auto master = session->master_out();
    if (!master) return 0.0f;

    auto meter = master->peak_meter();
    if (!meter) return 0.0f;

    float val = meter->meter_level(0, ARDOUR::MeterPeak);
    if (std::isnan(val) || std::isinf(val) || val < 0.0f) return 0.0f;
    return val;
}

float NovaAudioEngine::masterPeakRight() const
{
    auto session = m_sessionManager.session();
    if (!session) return 0.0f;

    auto master = session->master_out();
    if (!master) return 0.0f;

    auto meter = master->peak_meter();
    if (!meter) return 0.0f;

    float val = meter->meter_level(1, ARDOUR::MeterPeak);
    if (std::isnan(val) || std::isinf(val) || val < 0.0f) return 0.0f;
    return val;
}

float NovaAudioEngine::masterVolumeDb() const
{
    auto session = m_sessionManager.session();
    if (!session) return 0.0f;

    auto master = session->master_out();
    if (!master) return 0.0f;

    auto gc = master->gain_control();
    if (!gc) return 0.0f;

    float coeff = static_cast<float>(gc->get_value());
    if (coeff <= 0.0000001f) return -192.0f;
    return 20.0f * std::log10(coeff);
}

void NovaAudioEngine::setMasterVolumeDb(float dB)
{
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
}