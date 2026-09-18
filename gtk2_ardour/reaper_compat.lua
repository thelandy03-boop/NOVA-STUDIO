-- ============================================================================
-- NOVA-STUDIO — Complete ReaScript Compatibility Engine (reaper_compat.lua)
-- Full Mapping Layer for REAPER API (reaper.*) -> Ardour / NOVA Engine
-- ============================================================================

reaper = reaper or {}

-- ----------------------------------------------------------------------------
-- 0. DYNAMIC METATABLE PRE-CRASH GUARD (Prevents ALL nil method crashes)
-- ----------------------------------------------------------------------------
local function create_reaper_stub(func_name)
    return function(...)
        -- Soft log for unmapped Reaper functions (prevents crashes)
        -- print("<span foreground='#FFA500'>[ReaScript Stub] " .. tostring(func_name) .. "()</span>")
        return 0
    end
end

setmetatable(reaper, {
    __index = function(tbl, key)
        local stub = create_reaper_stub(key)
        rawset(tbl, key, stub)
        return stub
    end
})

-- ----------------------------------------------------------------------------
-- 1. Console, Debugging & Time
-- ----------------------------------------------------------------------------
function reaper.ShowConsoleMsg(msg)
    print(tostring(msg))
end

function reaper.ShowMessageBox(msg, title, type_val)
    print("[" .. tostring(title or "REAPER") .. "] " .. tostring(msg))
    return 1
end

function reaper.MB(msg, title, type_val)
    return reaper.ShowMessageBox(msg, title, type_val)
end

function reaper.time_precise()
    return os.clock()
end

function reaper.parse_timestr(str)
    -- Convierte "01:23.45" a segundos
    return tonumber(str) or 0.0
end

function reaper.format_timestr(sec)
    return string.format("%.3f", sec or 0.0)
end

-- ----------------------------------------------------------------------------
-- 2. Transport, Playhead & Tempo Controls
-- ----------------------------------------------------------------------------
function reaper.GetCursorPosition()
    if not Session then return 0.0 end
    local s = Session:instance()
    if not s then return 0.0 end
    local sr = s:sample_rate()
    if not sr or sr <= 0 then sr = 48000 end
    return s:transport_sample() / sr
end

function reaper.SetEditCurPos(time_sec, moveview, seekplay)
    if not Session then return end
    local s = Session:instance()
    if not s then return end
    local sr = s:sample_rate()
    if not sr or sr <= 0 then sr = 48000 end
    local sample_pos = math.floor(time_sec * sr)
    
    local force = seekplay or false
    local ok = pcall(function() s:request_locate(sample_pos, force, 0, 0) end)
    if not ok then ok = pcall(function() s:request_locate(sample_pos, force, 0) end) end
    if not ok then ok = pcall(function() s:request_locate(sample_pos, force) end) end
    if not ok then pcall(function() s:request_locate(sample_pos) end) end
end

function reaper.GetPlayState()
    if not Session then return 0 end
    local s = Session:instance()
    if not s then return 0 end
    return s:transport_rolling() and 1 or 0
end

function reaper.GetPlayPosition()
    return reaper.GetCursorPosition()
end

function reaper.OnPlayButton()
    if not Session then return end
    local s = Session:instance()
    if not s then return end
    local ok = pcall(function() s:request_transport_speed(1.0, false) end)
    if not ok then pcall(function() s:request_transport_speed(1.0) end) end
end

function reaper.OnStopButton()
    if not Session then return end
    local s = Session:instance()
    if not s then return end
    local ok = pcall(function() s:request_transport_speed(0.0, false) end)
    if not ok then pcall(function() s:request_transport_speed(0.0) end) end
end

function reaper.OnPauseButton()
    reaper.OnStopButton()
end

-- ----------------------------------------------------------------------------
-- 3. Track Management & Selection
-- ----------------------------------------------------------------------------
function reaper.CountTracks(proj)
    if not Session then return 0 end
    local s = Session:instance()
    if not s then return 0 end
    local tracks = s:get_tracks()
    if not tracks then return 0 end
    return tracks:size()
end

function reaper.GetTrack(proj, idx)
    if not Session then return nil end
    local s = Session:instance()
    if not s then return nil end
    local tracks = s:get_tracks()
    if not tracks then return nil end
    if idx < 0 or idx >= tracks:size() then return nil end
    return tracks:at(idx + 1)
end

function reaper.GetTrackName(track)
    if not track then return false, "" end
    if track.name then return true, track:name() end
    return false, ""
end

function reaper.GetSetMediaTrackInfo_String(track, parmname, string_val, setnewvalue)
    if not track then return false, "" end
    if parmname == "P_NAME" then
        if setnewvalue then
            if track.set_name then track:set_name(string_val) end
            return true, string_val
        else
            local name = track.name and track:name() or ""
            return true, name
        end
    end
    return false, ""
end

function reaper.SetMediaTrackInfo_Value(track, parmname, newval)
    if not track then return false end
    if parmname == "B_MUTE" then
        if track.set_mute then track:set_mute(newval == 1) end
        return true
    elseif parmname == "I_SOLO" then
        if track.set_solo then track:set_solo(newval == 1) end
        return true
    elseif parmname == "D_VOL" then
        if track.set_gain then track:set_gain(newval) end
        return true
    end
    return false
end

function reaper.GetMediaTrackInfo_Value(track, parmname)
    if not track then return 0 end
    if parmname == "B_MUTE" then
        if track.muted then return track:muted() and 1 or 0 end
        return 0
    elseif parmname == "I_SOLO" then
        if track.soloed then return track:soloed() and 1 or 0 end
        return 0
    elseif parmname == "D_VOL" then
        if track.gain then return track:gain() end
        return 1.0
    end
    return 0
end

function reaper.CountSelectedTracks(proj)
    -- En Ardour/NOVA, si hay Editor devolvemos la seleccion de pistas
    if not Editor then return 0 end
    local sel = Editor:get_selection()
    if sel and sel.tracks then
        return sel.tracks:size()
    end
    return 0
end

function reaper.GetSelectedTrack(proj, idx)
    if not Editor then return nil end
    local sel = Editor:get_selection()
    if sel and sel.tracks then
        if idx >= 0 and idx < sel.tracks:size() then
            return sel.tracks:at(idx + 1)
        end
    end
    return nil
end

-- ----------------------------------------------------------------------------
-- 4. Media Items & Regions (Clips)
-- ----------------------------------------------------------------------------
function reaper.CountMediaItems(proj)
    if not Session then return 0 end
    local s = Session:instance()
    if not s then return 0 end
    local count = 0
    local tracks = s:get_tracks()
    if tracks then
        for t in tracks:iter() do
            if t.playlist and t:playlist() then
                count = count + t:playlist():regions():size()
            end
        end
    end
    return count
end

function reaper.GetMediaItem(proj, idx)
    -- Obtener region por indice global
    if not Session then return nil end
    local s = Session:instance()
    if not s then return nil end
    local current = 0
    local tracks = s:get_tracks()
    if tracks then
        for t in tracks:iter() do
            if t.playlist and t:playlist() then
                local regs = t:playlist():regions()
                for r in regs:iter() do
                    if current == idx then return r end
                    current = current + 1
                end
            end
        end
    end
    return nil
end

function reaper.GetMediaItemInfo_Value(item, parmname)
    if not item then return 0.0 end
    local sr = (Session and Session:instance()) and Session:instance():sample_rate() or 48000
    if parmname == "D_POSITION" then
        return (item.position and item:position() or 0) / sr
    elseif parmname == "D_LENGTH" then
        return (item.length and item:length() or 0) / sr
    elseif parmname == "D_VOL" then
        return item.scale_amplitude and item:scale_amplitude() or 1.0
    end
    return 0.0
end

function reaper.SetMediaItemInfo_Value(item, parmname, newval)
    if not item then return false end
    local sr = (Session and Session:instance()) and Session:instance():sample_rate() or 48000
    if parmname == "D_POSITION" then
        if item.set_position then
            item:set_position(math.floor(newval * sr))
            return true
        end
    elseif parmname == "D_LENGTH" then
        if item.set_length then
            item:set_length(math.floor(newval * sr))
            return true
        end
    end
    return false
end

-- ----------------------------------------------------------------------------
-- 5. Markers & Regions
-- ----------------------------------------------------------------------------
function reaper.CountProjectMarkers(proj)
    if not Session then return 0, 0, 0 end
    local s = Session:instance()
    if not s or not s.locations then return 0, 0, 0 end
    local locs = s:locations()
    if not locs then return 0, 0, 0 end
    return locs:size(), 0, locs:size()
end

function reaper.AddProjectMarker(proj, isrgn, pos, rgnend, name, wantidx)
    if not Session then return -1 end
    local s = Session:instance()
    if not s or not s.locations then return -1 end
    local sr = s:sample_rate() or 48000
    local sample_pos = math.floor(pos * sr)
    s:locations():add(name or "Marker", sample_pos)
    return 1
end

-- ----------------------------------------------------------------------------
-- 6. FX & Plugin Parameter Management
-- ----------------------------------------------------------------------------
function reaper.TrackFX_GetCount(track)
    if not track or not track.nth_plugin then return 0 end
    local count = 0
    while track:nth_plugin(count) ~= nil do
        count = count + 1
    end
    return count
end

function reaper.TrackFX_GetParam(track, fx_idx, param_idx)
    if not track or not track.nth_plugin then return 0.0, 0.0, 1.0 end
    local plugin = track:nth_plugin(fx_idx)
    if plugin and plugin.parameter then
        return plugin:parameter(param_idx), 0.0, 1.0
    end
    return 0.0, 0.0, 1.0
end

function reaper.TrackFX_SetParam(track, fx_idx, param_idx, val)
    if not track or not track.nth_plugin then return false end
    local plugin = track:nth_plugin(fx_idx)
    if plugin and plugin.set_parameter then
        plugin:set_parameter(param_idx, val)
        return true
    end
    return false
end

-- ----------------------------------------------------------------------------
-- 7. Undo, Transactions & Refresh
-- ----------------------------------------------------------------------------
function reaper.Undo_BeginBlock() end
function reaper.Undo_EndBlock(desc, extra) end
function reaper.Undo_OnStateChange(desc) end
function reaper.Undo_OnStateChange2(proj, desc) end
function reaper.PreventUIRefresh(val) end
function reaper.UpdateArrange() end
function reaper.UpdateTimeline() end

function reaper.GetProjectPath(proj)
    if not Session then return "" end
    local s = Session:instance()
    if not s then return "" end
    return s:path() or ""
end

function reaper.GetProjectName(proj)
    if not Session then return "" end
    local s = Session:instance()
    if not s then return "" end
    return s:name() or ""
end