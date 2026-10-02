#ifndef NOVAANDROIDSTUBS_H
#define NOVAANDROIDSTUBS_H

#include <QtGlobal>

#if defined(Q_OS_ANDROID) || defined(Q_OS_WIN)

#include <string>
#include <vector>
#include <memory>
#include <algorithm>

namespace PBD {
    class Controllable {
    public:
        enum GroupControlDisposition { NoGroup };
        virtual ~Controllable() = default;
        virtual double get_value() const { return m_val; }
        virtual void set_value(double val, GroupControlDisposition = NoGroup) { m_val = val; }
    protected:
        double m_val = 1.0;
    };

    class ScopedConnectionList {
    public:
        void drop_connections() {}
    };

    class PropertyList {
    public:
        template<typename T, typename V>
        void add(T, V) {}
    };
}

namespace Temporal {
    class timepos_t {
    public:
        timepos_t(long long = 0) {}
        long long samples() const { return 0; }
    };
    class timecnt_t {
    public:
        timecnt_t(long long = 0) {}
        long long samples() const { return 0; }
    };
}

namespace ARDOUR {
    using samplepos_t = long long;
    using samplecnt_t = long long;
    using Sample = float;

    enum MonitorChoice { MonitorDisk };
    enum MeterType { MeterPeak };
    enum SrcQuality { SrcFastest };
    enum TrackMode { Normal, NonLayered, Destructive };

    namespace PresentationInfo {
        using order_t = uint32_t;
        inline const order_t max_order = 0xFFFFFFFF;
    }

    class AutomationControl : public PBD::Controllable {
    public:
        virtual ~AutomationControl() = default;
    };

    class GainControl : public AutomationControl {
    public:
        GainControl() { m_val = 1.0; }
    };

    class MuteControl : public AutomationControl {
    public:
        MuteControl() { m_val = 0.0; }
        bool muted() const { return m_val > 0.5; }
    };

    class SoloControl : public AutomationControl {
    public:
        SoloControl() { m_val = 0.0; }
        bool self_soloed() const { return m_val > 0.5; }
    };

    namespace Properties {
        inline const char* start = "start";
        inline const char* length = "length";
        inline const char* name = "name";
        inline const char* layer = "layer";
        inline const char* whole_file = "whole_file";
        inline const char* opaque = "opaque";
    }

    struct BusProfile {
        int master_out_channels = 2;
    };

    struct Config {
        void set_session_monitoring(MonitorChoice) {}
    };

    class Source {
    public:
        virtual ~Source() = default;
        Temporal::timecnt_t length() const { return Temporal::timecnt_t(0); }
        samplecnt_t read(Sample*, samplepos_t, samplecnt_t, int) { return 0; }
    };

    using SourceList = std::vector<std::shared_ptr<Source>>;

    struct ImportStatus {
        std::vector<std::string> paths;
        SrcQuality quality = SrcFastest;
        bool replace_existing_source = false;
        bool split_midi_channels = false;
        bool import_markers = false;
        bool cancel = false;
        bool done = true;
        bool all_done = true;
        int current = 0;
        int total = 1;
        SourceList sources;
    };

    class ID {
    public:
        std::string to_s() const { return "0"; }
    };

    class Region {
    public:
        virtual ~Region() = default;
        ID id() const { return ID(); }
        std::string name() const { return "Region"; }
        samplepos_t position_sample() const { return 0; }
        samplecnt_t length_samples() const { return 0; }
        void set_position(Temporal::timepos_t) {}
        void set_length(Temporal::timecnt_t) {}
    };

    class AudioRegion : public Region {
    public:
        float scale_amplitude() const { return 1.0f; }
        void set_scale_amplitude(float) {}
        bool fade_in_active() const { return false; }
        bool fade_out_active() const { return false; }
        Temporal::timecnt_t fade_in_length() const { return Temporal::timecnt_t(0); }
        Temporal::timecnt_t fade_out_length() const { return Temporal::timecnt_t(0); }
        void set_fade_in_active(bool) {}
        void set_fade_out_active(bool) {}
        void set_fade_in_length(samplecnt_t) {}
        void set_fade_out_length(samplecnt_t) {}
        uint32_t n_channels() const { return 2; }
        std::shared_ptr<Source> audio_source(uint32_t) { return nullptr; }
        samplepos_t start_sample() const { return 0; }
    };

    using RegionList = std::vector<std::shared_ptr<Region>>;

    class Playlist {
    public:
        std::shared_ptr<RegionList> region_list() { return std::make_shared<RegionList>(); }
        void add_region(std::shared_ptr<Region>, Temporal::timepos_t) {}
        void remove_region(std::shared_ptr<Region>) {}
        void split_region(std::shared_ptr<Region>, Temporal::timepos_t const &) {}
    };

    class Route {
    public:
        explicit Route(std::string name = "Master") : m_name(std::move(name)) {
            m_gain = std::make_shared<GainControl>();
            m_mute = std::make_shared<MuteControl>();
            m_solo = std::make_shared<SoloControl>();
            m_pan = std::make_shared<AutomationControl>();
            m_rec = std::make_shared<AutomationControl>();
        }
        virtual ~Route() = default;
        virtual bool is_track() const { return false; }
        virtual std::string name() const { return m_name; }

        std::shared_ptr<GainControl> gain_control() const { return m_gain; }
        std::shared_ptr<MuteControl> mute_control() const { return m_mute; }
        std::shared_ptr<SoloControl> solo_control() const { return m_solo; }
        std::shared_ptr<AutomationControl> pan_azimuth_control() const { return m_pan; }

    protected:
        std::string m_name;
        std::shared_ptr<GainControl> m_gain;
        std::shared_ptr<MuteControl> m_mute;
        std::shared_ptr<SoloControl> m_solo;
        std::shared_ptr<AutomationControl> m_pan;
        std::shared_ptr<AutomationControl> m_rec;
    };

    class Track : public Route {
    public:
        explicit Track(std::string name = "Track") : Route(std::move(name)) {}
        bool is_track() const override { return true; }
        std::shared_ptr<Playlist> playlist() { return std::make_shared<Playlist>(); }
        std::shared_ptr<AutomationControl> rec_enable_control() const { return m_rec; }
    };

    class AudioTrack : public Track {
    public:
        explicit AudioTrack(std::string name = "Audio Track") : Track(std::move(name)) {}
        std::shared_ptr<Source> write_source(int) { return nullptr; }
    };

    class MidiTrack : public Track {
    public:
        explicit MidiTrack(std::string name = "MIDI Track") : Track(std::move(name)) {}
    };

    using RouteList = std::vector<std::shared_ptr<Route>>;
    using AudioTrackList = std::vector<std::shared_ptr<AudioTrack>>;
    class RouteGroup {};

    class AudioBackend {
    public:
        virtual ~AudioBackend() = default;
    };

    class Session {
    public:
        Config config;
        Session(class AudioEngine&, const std::string& path, const std::string& name, BusProfile*, const std::string&, bool)
            : m_path(path), m_name(name), m_routes(std::make_shared<RouteList>()) {}
        
        bool dirty() const { return m_dirty; }
        void set_dirty() { m_dirty = true; }
        int save_state(const std::string&) { m_dirty = false; return 0; }
        
        std::string path() const { return m_path; }
        std::string name() const { return m_name; }
        double sample_rate() const { return 48000.0; }
        
        std::shared_ptr<RouteList> get_routes() { return m_routes; }
        void* master_out() { return nullptr; }
        void import_files(ImportStatus& s) { s.done = true; }

        AudioTrackList new_audio_track(int, int, std::shared_ptr<RouteGroup>, uint32_t how_many,
                                       std::string name_template, PresentationInfo::order_t,
                                       TrackMode = Normal, bool = true, bool = false) {
            AudioTrackList list;
            for (uint32_t i = 0; i < how_many; ++i) {
                auto trk = std::make_shared<AudioTrack>(name_template);
                m_routes->push_back(trk);
                list.push_back(trk);
            }
            m_dirty = true;
            return list;
        }

        void remove_route(std::shared_ptr<Route> r) {
            if (!r || !m_routes) return;
            auto it = std::find(m_routes->begin(), m_routes->end(), r);
            if (it != m_routes->end()) {
                m_routes->erase(it);
                m_dirty = true;
            }
        }

    private:
        std::string m_path;
        std::string m_name;
        bool m_dirty = false;
        std::shared_ptr<RouteList> m_routes;
    };

    class AudioEngine {
    public:
        static AudioEngine* create() { static AudioEngine eng; return &eng; }
        void set_backend(const std::string&, const std::string&, const std::string&) {}
        void* current_backend() { return this; }
        void set_sample_rate(float) {}
        void set_buffer_size(int) {}
        int start() { return 0; }
        void set_session(Session*) {}
    };

    class RegionFactory {
    public:
        static std::shared_ptr<Region> create(const SourceList&, const PBD::PropertyList&) {
            return std::make_shared<AudioRegion>();
        }
    };
}

#endif // Q_OS_ANDROID || Q_OS_WIN

#endif // NOVAANDROIDSTUBS_H