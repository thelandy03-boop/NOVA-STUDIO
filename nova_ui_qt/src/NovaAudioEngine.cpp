#include "NovaAudioEngine.h"
#include "models/NovaTrackListModel.h"
#include "models/NovaRegionModel.h"
#include "views/NovaWaveformItem.h"
#include <QDebug>

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
    connect(&m_transport, &NovaTransportController::bpmChanged, this, &NovaAudioEngine::bpmChanged);
    connect(&m_transport, &NovaTransportController::loopEnabledChanged, this, &NovaAudioEngine::loopEnabledChanged);
    connect(&m_transport, &NovaTransportController::loopRangeChanged, this, &NovaAudioEngine::loopRangeChanged);

    // Conectar señales del grabador
    connect(&m_recorder, &NovaRecordManager::isRecordingChanged, this, &NovaAudioEngine::isRecordingChanged);
}

bool NovaAudioEngine::initEngine()
{
    if (!m_sessionManager.initSession()) {
        return false;
    }

    auto session = m_sessionManager.session();

    m_transport.setSession(session);
    m_trackModel->setSession(session);
    m_regionModel->setSession(session);
    NovaWaveformItem::setSession(session);

    m_recorder.setSession(session);
    m_recorder.setRegionModel(m_regionModel);
    m_recorder.setTransportController(&m_transport);

    Q_EMIT tracksChanged();
    Q_EMIT regionsChanged();

    qDebug() << "🚀 [NOVA ENGINE] Arquitectura Modular Inicializada con Éxito.";
    return true;
}