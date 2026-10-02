#include "NovaAudioDeviceManager.h"
#include "NovaLogging.h"

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

#include "ardour/audioengine.h"
#include "ardour/audio_backend.h"

#pragma pop_macro("emit")
#pragma pop_macro("slots")
#pragma pop_macro("signals")
#pragma pop_macro("foreach")
#endif

NovaAudioDeviceManager::NovaAudioDeviceManager(QObject *parent)
    : QObject(parent)
{
#if defined(Q_OS_ANDROID) || defined(Q_OS_WIN)
    m_availableBackends << "AAudio" << "OpenSL ES" << "None (Dummy)";
    m_activeBackend = QStringLiteral("AAudio");
#elif defined(Q_OS_WIN)
    m_availableBackends << "WASAPI" << "ASIO" << "DirectSound" << "None (Dummy)";
    m_activeBackend = QStringLiteral("WASAPI");
#else
    m_availableBackends << "ALSA" << "JACK" << "PulseAudio" << "None (Dummy)";
#endif
}

void NovaAudioDeviceManager::setEngine(ARDOUR::AudioEngine *engine)
{
    m_engine = engine;
    if (m_engine) {
        refreshDevices();
    }
}

bool NovaAudioDeviceManager::switchBackend(const QString &backendName)
{
    if (!m_engine) return false;

    qCDebug(novaCore) << "Intentando cambiar backend a:" << backendName;

#if defined(Q_OS_ANDROID) || defined(Q_OS_WIN)
    m_activeBackend = backendName;
    Q_EMIT activeBackendChanged();
    refreshDevices();
    return true;
#else
    try {
        std::shared_ptr<ARDOUR::AudioBackend> newBackend = m_engine->set_backend(backendName.toStdString(), "NOVA-Studio", "");
        if (!newBackend) {
            qCWarning(novaCore) << "Imposible instanciar backend:" << backendName << ". Manteniendo actual:" << m_activeBackend;
            return false;
        }

        m_backend = newBackend;
        m_backend->set_sample_rate(static_cast<float>(m_sampleRate));
        m_backend->set_buffer_size(m_bufferSize);

        m_activeBackend = backendName;
        Q_EMIT activeBackendChanged();

        refreshDevices();
        qCDebug(novaCore) << "Backend cambiado con éxito a:" << m_activeBackend;
        return true;
    } catch (const std::exception &e) {
        qCCritical(novaCore) << "Excepción al cambiar backend:" << e.what();
        return false;
    }
#endif
}

void NovaAudioDeviceManager::refreshDevices()
{
    if (!m_engine) return;

    m_inputDevices.clear();
    m_outputDevices.clear();

#if defined(Q_OS_ANDROID) || defined(Q_OS_WIN)
    m_inputDevices << "Micrófono Android (AAudio)" << "Entrada de Audio Móvil";
    m_outputDevices << "Altavoces Android (AAudio)" << "Salida Auriculares / USB-C";
#elif defined(Q_OS_WIN)
    m_inputDevices << "Micrófono del Sistema (WASAPI)" << "Entrada DirectSound";
    m_outputDevices << "Altavoces / Auriculares (WASAPI)" << "Salida DirectSound";
#else
    auto currentBackend = m_engine->current_backend();
    if (currentBackend) {
        m_activeBackend = QString::fromStdString(m_engine->current_backend_name());

        if (currentBackend->use_separate_input_and_output_devices()) {
            auto inputs = currentBackend->enumerate_input_devices();
            for (const auto &dev : inputs) {
                if (dev.available) {
                    m_inputDevices.append(QString::fromStdString(dev.name));
                }
            }

            auto outputs = currentBackend->enumerate_output_devices();
            for (const auto &dev : outputs) {
                if (dev.available) {
                    m_outputDevices.append(QString::fromStdString(dev.name));
                }
            }
        } else {
            auto devices = currentBackend->enumerate_devices();
            for (const auto &dev : devices) {
                if (dev.available) {
                    QString devName = QString::fromStdString(dev.name);
                    m_inputDevices.append(devName);
                    m_outputDevices.append(devName);
                }
            }
        }
    }

    if (m_inputDevices.isEmpty()) {
        m_inputDevices << "Default Hardware Input" << "Micrófono Sistema (ALSA/PipeWire)";
    }
    if (m_outputDevices.isEmpty()) {
        m_outputDevices << "Default Hardware Output" << "Altavoces Sistema (ALSA/PipeWire)";
    }
#endif

    Q_EMIT devicesChanged();
    Q_EMIT activeBackendChanged();

    qCDebug(novaCore) << "Dispositivos actualizados. Backend:" << m_activeBackend 
                     << "| Entradas:" << m_inputDevices.size() 
                     << "| Salidas:" << m_outputDevices.size();
}

bool NovaAudioDeviceManager::setInputDevice(const QString &deviceName)
{
    if (!m_engine) return false;
#if defined(Q_OS_ANDROID) || defined(Q_OS_WIN)
    qCDebug(novaCore) << "Dispositivo de entrada establecido en:" << deviceName;
    return true;
#else
    auto backend = m_engine->current_backend();
    if (backend) {
        backend->set_input_device_name(deviceName.toStdString());
        qCDebug(novaCore) << "Dispositivo de entrada establecido en:" << deviceName;
        return true;
    }
    return false;
#endif
}

bool NovaAudioDeviceManager::setOutputDevice(const QString &deviceName)
{
    if (!m_engine) return false;
#if defined(Q_OS_ANDROID) || defined(Q_OS_WIN)
    qCDebug(novaCore) << "Dispositivo de salida establecido en:" << deviceName;
    return true;
#else
    auto backend = m_engine->current_backend();
    if (backend) {
        backend->set_output_device_name(deviceName.toStdString());
        qCDebug(novaCore) << "Dispositivo de salida establecido en:" << deviceName;
        return true;
    }
    return false;
#endif
}

bool NovaAudioDeviceManager::setSampleRate(int rate)
{
    if (rate <= 0 || m_sampleRate == rate) return false;
    m_sampleRate = rate;

    if (m_engine) {
        m_engine->set_sample_rate(m_sampleRate);
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_WIN)
        auto backend = m_engine->current_backend();
        if (backend) {
            backend->set_sample_rate(static_cast<float>(m_sampleRate));
        }
#endif
    }

    Q_EMIT audioConfigChanged();
    qCDebug(novaCore) << "Frecuencia de muestreo cambiada a:" << m_sampleRate << "Hz";
    return true;
}

bool NovaAudioDeviceManager::setBufferSize(int size)
{
    if (size <= 0 || m_bufferSize == size) return false;
    m_bufferSize = size;

    if (m_engine) {
        m_engine->set_buffer_size(m_bufferSize);
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_WIN)
        auto backend = m_engine->current_backend();
        if (backend) {
            backend->set_buffer_size(m_bufferSize);
        }
#endif
    }

    Q_EMIT audioConfigChanged();
    qCDebug(novaCore) << "Tamaño de buffer cambiado a:" << m_bufferSize << "samples";
    return true;
}