#ifndef NOVAANDROIDSTUBS_H
#define NOVAANDROIDSTUBS_H

#include <QtGlobal>

#if defined(Q_OS_ANDROID) || defined(Q_OS_WIN)

#include <string>
#include <vector>
#include <memory>

namespace PBD {
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
    };

    class Route {
    public:
        virtual ~Route() = default;
        virtual bool is_track() const { return false; }
        virtual std::string name() const { return "Master"; }
    };

    class Track : public Route {
    public:
        bool is_track() const override { return true; }
        std::shared_ptr<Playlist> playlist() { return std::make_shared<Playlist>(); }
    };

    class AudioTrack : public Track {
    public:
        std::shared_ptr<Source> write_source(int) { return nullptr; }
    };

    using RouteList = std::vector<std::shared_ptr<Route>>;

    class AudioBackend {
    public:
        virtual ~AudioBackend() = default;
    };

    class Session {
    public:
        Config config;
        Session(class AudioEngine&, const std::string& path, const std::string& name, BusProfile*, const std::string&, bool)
            : m_path(path), m_name(name) {}
        
        bool dirty() const { return m_dirty; }
        void set_dirty() { m_dirty = true; }
        int save_state(const std::string&) { m_dirty = false; return 0; }
        
        std::string path() const { return m_path; }
        std::string name() const { return m_name; }
        double sample_rate() const { return 48000.0; }
        
        std::shared_ptr<RouteList> get_routes() { return std::make_shared<RouteList>(); }
        void* master_out() { return nullptr; }
        void import_files(ImportStatus& s) { s.done = true; }

    private:
        std::string m_path;
        std::string m_name;
        bool m_dirty = false;
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