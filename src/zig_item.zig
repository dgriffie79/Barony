const c = @cImport({
    @cInclude("items.h");
});

export fn itemIsThrowableTinkerTool(item: ?*const c.Item) bool {
    const it = item orelse return false;
    const t = it.type;
    return (t >= c.TOOL_BOMB and t <= c.TOOL_TELEPORT_BOMB) or
        t == c.TOOL_DECOY or
        t == c.TOOL_SENTRYBOT or
        t == c.TOOL_SPELLBOT or
        t == c.TOOL_GYROBOT or
        t == c.TOOL_DUMMYBOT;
}
