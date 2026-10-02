#include "NovaRecordManager.h"
#include "NovaLogging.h"
#include "../models/NovaRegionModel.h"
#include "NovaTransportController.h"
#include <QDebug>
#include <algorithm>

#if defined(Q_OS_ANDROID) || defined(Q_OS_WIN)
#include "platform/NovaAndroidStubs.h"
#else
#pragma push_macro("emit")
#pragma push_macro("slots")
#pragma push_macro("signals")
#pragma push_macro("foreach")
#undef emit
#undef slots
#undef signals
#undef foreach

#include "ardour/session.h"
#include "ardour/track.h"
#include "ardour/route.h"

#pragma pop_macro("emit")
#pragma pop_macro("slots")
#pragma pop_macro("signals")
#pragma pop_macro("foreach")
#endif

NovaRecordManager::NovaRecordManager(QObject *parent)
    : QObject(parent)
{
}

void NovaRecordManager::setSession(ARDOUR::Session *session)
{
    m_session = session;
}

void NovaRecordManager::setRegionModel(NovaRegionModel *regionModel)
{
    m_regionModel = regionModel;
}

void NovaRecordManager::setTransportController(NovaTransportController *transport)
{
    m_transport = transport;
    if (m_transport) {
        connect(m_transport, &NovaTransportController::positionChanged,
                this, &NovaRecordManager::onPositionChanged);
        connect(m_transport, &NovaTransportController::transportStoppedWasRecording,
                this, &NovaRecordManager::onTransportStopped);
    }
}

void NovaRecordManager::toggleRecord()
{
    if (!m_session) {
        qCWarning(novaRecord) << "Sin sesión activa.";
        return;
    }

#if defined(Q_OS_ANDROID) || defined(Q_OS_WIN)
    const bool alreadyRecording = m_isRecording || m_liveRegionActive;

    if (alreadyRecording) {
        m_isRecording = false;
        m_liveRegionActive = false;

        if (m_transport) {
            m_transport->stop();
        }

        if (m_regionModel) {
            m_regionModel->finalizeLiveRecordingClip();
        }

        Q_EMIT isRecordingChanged();
        qCDebug(novaRecord) << "Grabación detenida limpiamente (STOP Stub).";
        return;
    }

    m_isRecording = true;
    m_liveRegionActive = false;

    if (m_transport) {
        m_transport->play();
    }

    Q_EMIT isRecordingChanged();
    qCDebug(novaRecord) << "Grabación en vivo iniciada (ROLL Stub).";
#else
    const bool alreadyRecording = m_isRecording || m_session->get_record_enabled() || m_liveRegionActive;

    if (alreadyRecording) {
        m_session->disable_record(false, false);
        m_isRecording = false;
        m_liveRegionActive = false;

        if (m_transport) {
            m_transport->stop();
        }

        if (m_regionModel) {
            m_regionModel->finalizeLiveRecordingClip();
        }

        Q_EMIT isRecordingChanged();
        qCDebug(novaRecord) << "Grabación detenida limpiamente (STOP).";
        return;
    }

    bool anyArmed = false;
    if (auto routes = m_session->get_routes()) {
        for (auto &route : *routes) {
            if (!route || !route->is_track()) continue;
            auto track = std::dynamic_pointer_cast<ARDOUR::Track>(route);
            if (!track) continue;
            auto rec = track->rec_enable_control();
            if (rec && rec->get_value() != 0.0) {
                anyArmed = true;
                break;
            }
        }

        if (!anyArmed) {
            for (auto &route : *routes) {
                if (!route || !route->is_track()) continue;
                auto track = std::dynamic_pointer_cast<ARDOUR::Track>(route);
                if (!track) continue;
                auto rec = track->rec_enable_control();
                if (rec) {
                    rec->set_value(1.0, PBD::Controllable::NoGroup);
                    qCDebug(novaRecord) << "Auto-arm de seguridad en pista:" << QString::fromStdString(track->name());
                    anyArmed = true;
                    break;
                }
            }
        }
    }

    m_session->maybe_enable_record();
    m_isRecording = true;
    m_liveRegionActive = false;

    if (m_transport) {
        m_transport->play();
    }

    Q_EMIT isRecordingChanged();
    qCDebug(novaRecord) << "Grabación en vivo iniciada (ROLL).";
#endif
}

void NovaRecordManager::onPositionChanged()
{
    if (!m_session || !m_isRecording || !m_transport) return;
    if (!m_transport->isPlaying()) return;

    double currentBeat = m_transport->currentBeat();

    if (!m_liveRegionActive) {
        m_recordStartBeat = currentBeat;
        m_liveRegionActive = true;

        if (m_regionModel) {
            m_regionModel->createLiveRecordingClip(m_recordStartBeat);
            qCDebug(novaRecord) << "Clip de grabación en vivo creado en beat:" << m_recordStartBeat;
        }
    } else if (m_regionModel) {
        double lengthBeats = std::max(0.0, currentBeat - m_recordStartBeat);
        m_regionModel->updateLiveRecordingClip(lengthBeats);
    }
}

void NovaRecordManager::onTransportStopped()
{
    if (!m_liveRegionActive && !m_isRecording) return;

    qCDebug(novaRecord) << "Transporte detenido. Finalizando clip en vivo...";
    m_liveRegionActive = false;
    m_isRecording = false;

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_WIN)
    if (m_session && m_session->get_record_enabled()) {
        m_session->disable_record(false, false);
    }
#endif

    if (m_regionModel) {
        m_regionModel->finalizeLiveRecordingClip();
    }

    Q_EMIT isRecordingChanged();
}