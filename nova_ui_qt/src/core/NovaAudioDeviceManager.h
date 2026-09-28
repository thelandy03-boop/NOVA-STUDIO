#ifndef NOVAAUDIODEVICEMANAGER_H
#define NOVAAUDIODEVICEMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <memory>

#pragma push_macro("emit")
#pragma push_macro("slots")
#pragma push_macro("signals")
#pragma push_macro("foreach")
#undef emit
#undef slots
#undef signals
#undef foreach

namespace ARDOUR {
    class AudioEngine;
    class AudioBackend;
}

#pragma pop_macro("emit")
#pragma pop_macro("slots")
#pragma pop_macro("signals")
#pragma pop_macro("foreach")

class NovaAudioDeviceManager : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString activeBackend READ activeBackend NOTIFY activeBackendChanged)
    Q_PROPERTY(QStringList availableBackends READ availableBackends NOTIFY availableBackendsChanged)
    Q_PROPERTY(QStringList inputDevices READ inputDevices NOTIFY devicesChanged)
    Q_PROPERTY(QStringList outputDevices READ outputDevices NOTIFY devicesChanged)
    Q_PROPERTY(int sampleRate READ sampleRate WRITE setSampleRate NOTIFY audioConfigChanged)
    Q_PROPERTY(int bufferSize READ bufferSize WRITE setBufferSize NOTIFY audioConfigChanged)

public:
    explicit NovaAudioDeviceManager(QObject *parent = nullptr);
    ~NovaAudioDeviceManager() override = default;

    void setEngine(ARDOUR::AudioEngine *engine);

    // 🎛️ Métodos expuestos a QML
    Q_INVOKABLE bool switchBackend(const QString &backendName);
    Q_INVOKABLE void refreshDevices();
    Q_INVOKABLE bool setInputDevice(const QString &deviceName);
    Q_INVOKABLE bool setOutputDevice(const QString &deviceName);
    Q_INVOKABLE bool setSampleRate(int rate);
    Q_INVOKABLE bool setBufferSize(int size);

    QString activeBackend() const { return m_activeBackend; }
    QStringList availableBackends() const { return m_availableBackends; }
    QStringList inputDevices() const { return m_inputDevices; }
    QStringList outputDevices() const { return m_outputDevices; }
    int sampleRate() const { return m_sampleRate; }
    int bufferSize() const { return m_bufferSize; }

signals:
    void activeBackendChanged();
    void availableBackendsChanged();
    void devicesChanged();
    void audioConfigChanged();

private:
    ARDOUR::AudioEngine *m_engine = nullptr;
    std::shared_ptr<ARDOUR::AudioBackend> m_backend;

    QString m_activeBackend = "None (Dummy)";
    QStringList m_availableBackends;
    QStringList m_inputDevices;
    QStringList m_outputDevices;

    int m_sampleRate = 44100;
    int m_bufferSize = 512;
};

#endif // NOVAAUDIODEVICEMANAGER_H