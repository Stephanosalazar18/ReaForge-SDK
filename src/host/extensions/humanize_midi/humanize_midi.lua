local M = {}

function M.run(ctx)
    if not ctx or not ctx.item then
        return { ok = false, error = "humanize_midi: no item in context" }
    end

    local item = ctx.item
    local ok, notes = pcall(reaper.MIDI_GetNotes, item)
    if not ok then
        return { ok = false, error = "humanize_midi: MIDI_GetNotes failed" }
    end
    if type(notes) ~= "table" or #notes == 0 then
        return { ok = true, count = 0 }
    end

    local timing_ms = tonumber(ctx.timing_ms) or 12
    local vel_amt   = tonumber(ctx.velocity)   or 8

    for _, n in ipairs(notes) do
        n.pos = n.pos + (math.random() - 0.5) * (timing_ms / 1000.0)
        local v = math.floor(n.vel + (math.random() - 0.5) * vel_amt + 0.5)
        if v < 1   then v = 1   end
        if v > 127 then v = 127 end
        n.vel = v
    end

    pcall(reaper.MIDI_SetNotes, item, notes, nil, nil, "Humanize MIDI")
    return { ok = true, count = #notes }
end

return M
