#include "NovaAudioEngine.h"
#include "core/NovaLogging.h"
#include "core/NovaAudioDeviceManager.h"
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
    m_recorder.setSession(nullptr);
}

bool NovaAudioEngine::initEngine()
{
    if (!m_sessionManager.initSession()) {
        qCCritical(novaCore) << "Fallo al inicializar la sesión de audio.";
        return false;
    }

    auto session = m_sessionManager.session();
    auto engine = m_sessionManager.engine();

    m_deviceManager.setEngine(engine);
    m_transport.setSession(session);
    m_trackModel->setSession(session);
    m_regionModel->setSession(session);
    NovaWaveformItem::setSession(session);

    m_recorder.setSession(session);
    m_recorder.setRegionModel(m_regionModel);
    m_recorder.setTransportController(&m_transport);

    Q_EMIT tracksChanged();
    Q_EMIT regionsChanged();

    qCDebug(novaCore) << "🚀 Arquitectura Modular Inicializada con Éxito con Motor Real.";
    return true;
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
    return std::clamp(val, 0.0f, 1.5f);
}

float NovaAudioEngine::masterPeakRight() const
{
    auto session = m_sessionManager.session();
    if (!session) return 0.0f;
    auto master = session->master_out();
    if (!master) return 0.0f;
    auto meter = master->peak_meter();
    if (!meter) return 0.0f;

    // Si el Master Bus es estéreo, leer canal 1 (Derecho), de lo contrario canal 0
    uint32_t channel = (meter->input_streams().n_audio() > 1) ? 1 : 0;
    float val = meter->meter_level(channel, ARDOUR::MeterPeak);
    if (std::isnan(val) || std::isinf(val) || val < 0.0f) return 0.0f;
    return std::clamp(val, 0.0f, 1.5f);
}

float NovaAudioEngine::masterVolumeDb() const
{
    auto session = m_sessionManager.session();
    if (!session) return 0.0f;
    auto master = session->master_out();
    if (!master || !master->gain_control()) return 0.0f;

    float coeff = static_cast<float>(master->gain_control()->get_value());
    if (coeff <= 0.0000001f) return -192.0f;
    return 20.0f * std::log10(coeff);
}

void NovaAudioEngine::setMasterVolumeDb(float dB)
{
    auto session = m_sessionManager.session();
    if (!session) return;
    auto master = session->master_out();
    if (!master || !master->gain_control()) return;

    float coeff = (dB <= -192.0f) ? 0.0f : std::pow(10.0f, dB / 20.0f);
    master->gain_control()->set_value(coeff, PBD::Controllable::NoGroup);
    Q_EMIT masterVolumeDbChanged();
}