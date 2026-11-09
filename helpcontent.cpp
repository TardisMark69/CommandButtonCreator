// helpcontent.cpp — every help page. Plain HTML (the subset QTextBrowser
// understands). To change wording, edit the page here and rebuild.
#include "helpcontent.h"

namespace HelpContent {

namespace {

QList<HelpTopic> build()
{
    QList<HelpTopic> t;
    auto add = [&t](const char *id, const char *parent, const char *title,
                    QStringList keywords, const char *html) {
        t.append({QString::fromLatin1(id), QString::fromLatin1(parent),
                  QString::fromUtf8(title), keywords, QString::fromUtf8(html)});
    };

    // ===================================================================
    add("welcome", "", "Welcome",
        {QStringLiteral("start"), QStringLiteral("overview"), QStringLiteral("introduction")},
        R"HTML(
<h1>Command Button Creator</h1>
<p>One press instead of opening a terminal and typing. Each button holds a
command; click the button and the command runs.</p>
<h2>How to use this help</h2>
<ul>
<li><b>Contents</b> (left) — every page, grouped. Click a heading to open it.</li>
<li><b>Index</b> — an alphabetical list of keywords. Start typing to narrow it,
then press Enter.</li>
<li><b>Search</b> — type words; you get every page that contains all of them.</li>
<li><b>Back / Forward / Contents</b> — walk your trail like a web browser.
<b>A−</b> and <b>A+</b> change the text size of these pages.</li>
</ul>
<h2>First stops</h2>
<ul>
<li><a href="topic:whatsnew">What's new in 2.0</a> — including two old problems
that are now fixed.</li>
<li><a href="topic:quickstart">Getting started</a> — the five minutes worth spending
first.</li>
<li><a href="topic:modes">The three run modes</a> — the most important new idea.</li>
<li><a href="topic:secrets">The secrets vault</a> — keep tokens out of commands.</li>
<li><a href="topic:shortcuts">Keyboard shortcuts</a>.</li>
</ul>
<p><i>Press <span class="key">F1</span> anywhere in the program to open this help.
Every dialog also has a <b>Help</b> button that opens the right page, and resting
the mouse on any control shows a short explanation.</i></p>
)HTML");

    // ===================================================================
    add("whatsnew", "welcome", "What's new in 2.0",
        {QStringLiteral("new features"), QStringLiteral("version 2"), QStringLiteral("changes"),
         QStringLiteral("fixed")},
        R"HTML(
<h1>What's new in 2.0</h1>
<h2>Fixed</h2>
<ul>
<li><b>Commands ending in &amp; never ran.</b> Version 1 added
<code>; echo 'Press Enter to close...'; read</code> after every command. When a command
ended in <code>&amp;</code> (like <i>Restart Ollama</i>) that made
<code>&amp;;</code>, which bash rejects, so the terminal showed an error and closed.
2.0 puts the closing message on its own line, so any command works.</li>
<li><b>Buttons are no longer stored beside the program.</b> They now live in your
settings folder (see <a href="topic:files">Where files are kept</a>). Your old
<code>buttons.json</code> was copied across the first time 2.0 started and left
untouched.</li>
</ul>
<h2>New</h2>
<ul>
<li><a href="topic:modes">Three run modes</a>: Terminal window, Output panel
(quiet, with ✓/✗ on the button), and Background (keeps running after you close
this program).</li>
<li><a href="topic:statuslights">Status lights</a>: a green or red light that shows
whether the thing a button controls is running.</li>
<li><a href="topic:secrets">Secrets vault</a> and
<a href="topic:securitycheck">Security check</a>: tokens and passwords come out of
commands in one click.</li>
<li><a href="topic:checker">Command checker</a>: explains problems before you save.</li>
<li><a href="topic:pages">Pages</a> (tabs) and <a href="topic:editing">colors</a> to
organize buttons.</li>
<li><a href="topic:undo">Undo and redo</a>, automatic <a href="topic:backups">backups</a>,
and <a href="topic:lock">Lock layout</a> against accidental drags.</li>
<li><a href="topic:arranging">Box selection, moving several buttons at once, snap to
grid and arrow-key nudging</a>.</li>
<li><a href="topic:importexport">Import and export</a>, a
<a href="topic:launcher">desktop launcher</a>, and
<a href="topic:zoom">Ctrl+wheel zoom</a>.</li>
</ul>
)HTML");

    // ===================================================================
    add("quickstart", "welcome", "Getting started",
        {QStringLiteral("first steps"), QStringLiteral("setup"), QStringLiteral("tutorial")},
        R"HTML(
<h1>Getting started</h1>
<p>Your buttons from version 1 are already here, all set to <b>Terminal window</b>
mode, so they behave as before. Five things are worth doing once:</p>
<h2>1. Run the Security check</h2>
<p><b>Tools → Security check…</b> finds passwords and tokens typed into commands
(for example a Home Assistant token) and moves them into the
<a href="topic:secrets">vault</a> with one click. If the status bar shows a yellow
<span class="warn">⚠</span> message, click it — it opens the same check.</p>
<h2>2. Give quick commands the Output panel mode</h2>
<p>Buttons that just <i>do</i> something — lights on, restart a service, check a
temperature — don't need a terminal. Right-click one → <b>Runs in → Output
panel</b>. Now it runs without a window and shows ✓ or ✗. See
<a href="topic:modes">The three run modes</a>.</p>
<h2>3. Give servers the Background mode</h2>
<p>Buttons that start something that should keep running (a server, a web UI)
belong in <b>Background</b> mode — and drop any <code>&amp;</code> at the end.</p>
<h2>4. Add a status light or two</h2>
<p>Edit a button and fill in <b>Status light</b>, e.g. <code>pgrep -x ollama</code>.
See <a href="topic:statuslights">Status lights</a> for ready-made checks.</p>
<h2>5. Lock the layout</h2>
<p>When the buttons are where you want them, press <b>🔒 Lock</b> (Ctrl+L) so a
shaky drag can never move one.</p>
)HTML");

    // ===================================================================
    add("mainwindow", "", "The main window",
        {QStringLiteral("window"), QStringLiteral("toolbar"), QStringLiteral("menus"),
         QStringLiteral("status bar"), QStringLiteral("layout of the screen")},
        R"HTML(
<h1>The main window</h1>
<ul>
<li><b>Menus</b> — File, Edit, Arrange, View, Tools, Help. Every command is in a
menu, with its shortcut beside it.</li>
<li><b>Toolbar</b> — the everyday commands: <b>Add Button</b>, <b>Organize</b>,
<b>Make All Same Size</b>, <b>🔒 Lock</b>, <b>Output</b>, <b>🔑 Secrets</b>,
<b>Undo</b> and <b>Help</b>.</li>
<li><b>Page tabs</b> — one tab per <a href="topic:pages">page</a>. The <b>+</b> at the
right adds a page.</li>
<li><b>The canvas</b> — the dark area holding the buttons. While the layout is
unlocked it shows a faint dot grid.</li>
<li><b>The Output panel</b> — appears at the bottom when needed; see
<a href="topic:outputpanel">The Output panel</a>.</li>
<li><b>The status bar</b> — messages on the left; on the right the number of
buttons, how many commands are running, and a yellow
<span class="warn">⚠</span> warning if any command holds a typed-in secret
(click it to fix).</li>
</ul>
<p>The window can be resized, minimized and maximized, and remembers its size and
position.</p>
)HTML");

    add("button", "mainwindow", "Reading a button",
        {QStringLiteral("light"), QStringLiteral("badge"), QStringLiteral("tick"),
         QStringLiteral("cross"), QStringLiteral("spinner"), QStringLiteral("tooltip"),
         QStringLiteral("glyph")},
        R"HTML(
<h1>Reading a button</h1>
<ul>
<li><b>Top-left light</b> — only if the button has a
<a href="topic:statuslights">status check</a>: <span style="color:#3ddc84">● green</span>
running, <span style="color:#ff5252">● red</span> not running, grey while first checking.</li>
<li><b>Top-right badge</b> — the last run: an orange spinner while an Output-panel
command is running, a green ✓ when it worked (fades after a few seconds), a red ✗
when it failed (stays until the next run).</li>
<li><b>Bottom-left mark</b> — the run mode: <code>&gt;_</code> Terminal window,
<code>≡</code> Output panel, <code>↗</code> Background.</li>
<li><b>Gold border</b> — the button is selected.</li>
<li><b>Grip lines bottom-right</b> — drag them to resize (only when unlocked).</li>
</ul>
<p>Rest the mouse on a button to see its mode, last result, status, and the first
lines of its command.</p>
)HTML");

    // ===================================================================
    add("running", "", "Running a button",
        {QStringLiteral("run"), QStringLiteral("click"), QStringLiteral("confirm"),
         QStringLiteral("ask before running"), QStringLiteral("double click"),
         QStringLiteral("already running")},
        R"HTML(
<h1>Running a button</h1>
<p><b>Click it.</b> That's all. With the keyboard: Tab to it and press
<span class="key">Enter</span> or <span class="key">Space</span>.</p>
<h2>Safety catches</h2>
<ul>
<li><b>Accidental double clicks are ignored.</b> A second click on the same button
within 0.7 seconds doesn't run it twice.</li>
<li><b>Ask before running.</b> Tick this in the <a href="topic:editing">Edit
dialog</a> and the button asks “Run …?” first, showing the command. Enter says yes,
Escape says no.</li>
<li><b>Already running.</b> If an Output-panel button is still running when you
click it again, you're asked whether to stop it, start another copy, or leave it.</li>
<li><b>Drags never run.</b> If you press a button and move the mouse more than a few
pixels, it's treated as a drag — even when the layout is locked — so a shaky press
won't fire it.</li>
</ul>
<h2>Where the result shows</h2>
<p>That depends on the <a href="topic:modes">run mode</a>. The button's badge and the
status bar always say what happened.</p>
)HTML");

    // ===================================================================
    add("modes", "", "The three run modes",
        {QStringLiteral("mode"), QStringLiteral("runs in"), QStringLiteral("which mode")},
        R"HTML(
<h1>The three run modes</h1>
<p>Every button runs its command in one of three ways. Pick it in the Edit dialog
(<b>Runs in</b>) or right-click → <b>Runs in</b>.</p>
<table border="1" cellspacing="0">
<tr><th>Mode</th><th>Use it for</th><th>You see</th></tr>
<tr><td><a href="topic:mode_terminal">&gt;_ Terminal window</a></td>
<td>Anything you type into: <code>ssh</code>, <code>ollama run</code>,
<code>sudo</code>, scripts that ask questions</td>
<td>A terminal that stays open until you press Enter</td></tr>
<tr><td><a href="topic:mode_output">≡ Output panel</a></td>
<td>Commands that just do something or report something: lights,
<code>docker restart</code>, <code>systemctl</code>, GPU temperature</td>
<td>✓ or ✗ on the button; the text in the Output panel</td></tr>
<tr><td><a href="topic:mode_background">↗ Background</a></td>
<td>Servers and services that should keep running: Open WebUI, a node server</td>
<td>↗ in the Output list; the text goes to a log</td></tr>
</table>
<h2>How to choose</h2>
<p>Will you need to type into it? → <b>Terminal window</b>.<br>
Should it keep running after you close this program? → <b>Background</b>.<br>
Otherwise → <b>Output panel</b>.</p>
<p class="tip">The <a href="topic:checker">command checker</a> warns you if a mode
doesn't suit a command (for example <code>sudo</code> in Output-panel mode).</p>
)HTML");

    add("mode_terminal", "modes", "Terminal window mode",
        {QStringLiteral("terminal"), QStringLiteral("gnome-terminal"), QStringLiteral("press enter to close"),
         QStringLiteral("interactive"), QStringLiteral("sudo")},
        R"HTML(
<h1>&gt;_ Terminal window mode</h1>
<p>Opens a terminal and runs the command in it, exactly as if you had typed it. The
window title is the button's label. When the command ends, a line shows its
<b>exit code</b> (0 means it worked) and the window waits for
<span class="key">Enter</span> so you can read the result.</p>
<p>Use this mode whenever you need to type: logging in with <code>ssh</code>,
chatting with <code>ollama run</code>, entering a <code>sudo</code> password.</p>
<h2>Details</h2>
<ul>
<li>The command runs inside <code>( … )</code>, so <code>exit</code>, a trailing
<code>&amp;</code> or a comment on the last line can't stop the closing message from
appearing.</li>
<li>Something started with <code>&amp;</code> in a terminal is stopped when you close
that terminal. To keep it running use <a href="topic:mode_background">Background</a>.</li>
<li>Which terminal program is used is set in
<a href="topic:preferences">Preferences</a> (GNOME Terminal on Pop!_OS).</li>
</ul>
)HTML");

    add("mode_output", "modes", "Output panel mode",
        {QStringLiteral("output"), QStringLiteral("quiet"), QStringLiteral("exit code"),
         QStringLiteral("stop"), QStringLiteral("captured")},
        R"HTML(
<h1>≡ Output panel mode</h1>
<p>Runs the command with no window. While it runs, the button shows an orange
spinner; when it ends, a green ✓ (exit code 0) or a red ✗ (anything else). What
it printed appears in the <a href="topic:outputpanel">Output panel</a>, which opens
by itself (you can turn that off in <a href="topic:preferences">Preferences</a>).</p>
<h2>Details</h2>
<ul>
<li>Nothing can be typed into the command — anything that asks a question gets an
empty answer. Use Terminal window mode for those.</li>
<li><b>Stop</b> in the Output panel ends the command <i>and everything it started</i>;
if something ignores the polite request, it's ended firmly after 3 seconds.</li>
<li>The last 1 MB of output is kept. Colour codes and progress-bar redraws are
cleaned up so the text reads properly.</li>
<li>Secrets from the <a href="topic:secrets">vault</a> are blanked out of what's
shown.</li>
<li>If you close the program while a command is running, you're asked first,
because it will be stopped.</li>
</ul>
)HTML");

    add("mode_background", "modes", "Background mode",
        {QStringLiteral("background"), QStringLiteral("server"), QStringLiteral("service"),
         QStringLiteral("log"), QStringLiteral("keep running"), QStringLiteral("detached")},
        R"HTML(
<h1>↗ Background mode</h1>
<p>Starts the command and lets it go. It keeps running when you close this program
or its terminal — the right mode for servers such as Open WebUI or a web app.</p>
<p>Everything it prints goes to a <b>log</b> for that button. To read it:
right-click the button → <b>Show log</b>, or select its run in the Output panel.
<b>Open log</b> opens it in your text editor.</p>
<h2>Details</h2>
<ul>
<li>Don't end the command with <code>&amp;</code> — Background mode already does that
job, properly.</li>
<li>Each start adds a dated line to the log. When a log passes 2 MB it is renamed
<code>.log.1</code> and a fresh one begins.</li>
<li>This program doesn't stop Background commands. Stop them the usual way (another
button, e.g. <code>pkill -f open-webui</code>).</li>
</ul>
)HTML");

    // ===================================================================
    add("outputpanel", "", "The Output panel",
        {QStringLiteral("output"), QStringLiteral("history"), QStringLiteral("results"),
         QStringLiteral("copy output"), QStringLiteral("clear"), QStringLiteral("F9")},
        R"HTML(
<h1>The Output panel</h1>
<p>Open or close it with <b>View → Output panel</b>, the <b>Output</b> toolbar
button, or <span class="key">F9</span>. You can drag it to another edge or float it
as its own window.</p>
<p><b>Left:</b> every run this session, newest at the bottom —
<span style="color:#ffb366">●</span> running,
<span style="color:#8fe3aa">✓</span> worked,
<span style="color:#ff8a80">✗</span> failed,
▣ opened in a terminal, ↗ started in the background. “Test:” marks a test run from
the Edit dialog.</p>
<p><b>Right:</b> the selected run — label, mode, start time, result and how long it
took — and what it printed. For a Background run, the end of its log.</p>
<h2>Buttons</h2>
<ul>
<li><b>■ Stop</b> — stop the selected run (Output-panel mode only).</li>
<li><b>Copy</b> — copy the text shown. Secrets stay blanked out.</li>
<li><b>Open log</b> — open a Background button's log in your text editor.</li>
<li><b>Clear finished</b> — tidy the list. Running commands stay.</li>
</ul>
<p>The list keeps the last 200 runs and is not saved between sessions.</p>
)HTML");

    // ===================================================================
    add("statuslights", "", "Status lights",
        {QStringLiteral("status"), QStringLiteral("light"), QStringLiteral("green"),
         QStringLiteral("red"), QStringLiteral("is it running"), QStringLiteral("pgrep"),
         QStringLiteral("refresh"), QStringLiteral("F5")},
        R"HTML(
<h1>Status lights</h1>
<p>A button can show whether the thing it controls is running. In the Edit dialog,
fill in <b>Status light</b> with a short command that <b>succeeds when it's up</b>.
Exit code 0 → <span style="color:#3ddc84">● green</span>; anything else →
<span style="color:#ff5252">● red</span>. Press <b>Try</b> to test it on the spot.</p>
<h2>Ready-made checks</h2>
<table border="1" cellspacing="0">
<tr><th>To show that…</th><th>Status light</th></tr>
<tr><td>Ollama is running here</td><td><code>pgrep -x ollama</code></td></tr>
<tr><td>Ollama answers on its port</td><td><code>curl -sf http://127.0.0.1:11434/api/version</code></td></tr>
<tr><td>A Docker container is up</td><td><code>docker ps -q -f name=homeassistant -f status=running | grep -q .</code></td></tr>
<tr><td>A user service is active</td><td><code>systemctl --user is-active --quiet syncthing</code></td></tr>
<tr><td>A web page answers</td><td><code>curl -sf -o /dev/null http://127.0.0.1:3000</code></td></tr>
<tr><td>Another computer is reachable</td><td><code>ping -c1 -W2 100.83.36.84</code></td></tr>
<tr><td>A program is running (by its command line)</td><td><code>pgrep -f open-webui</code></td></tr>
</table>
<h2>When lights are checked</h2>
<ul>
<li>Every 30 seconds (change it in <a href="topic:preferences">Preferences</a>).</li>
<li>A few seconds after the button runs, and again a little later.</li>
<li>Whenever you press <span class="key">F5</span> (Tools → Refresh status lights).</li>
<li>Not while the window is minimized.</li>
</ul>
<p>At most four checks run at once, and each is stopped after 8 seconds (which
counts as red). Rest the mouse on a button to see when its light was checked and
the last thing the check printed.</p>
<p class="warn">Status checks have no terminal, so they can't use <code>sudo</code>.</p>
)HTML");

    // ===================================================================
    add("editing", "", "Making and editing buttons",
        {QStringLiteral("add button"), QStringLiteral("new button"), QStringLiteral("edit"),
         QStringLiteral("label"), QStringLiteral("command"), QStringLiteral("color"),
         QStringLiteral("duplicate"), QStringLiteral("delete"), QStringLiteral("test run"),
         QStringLiteral("insert secret")},
        R"HTML(
<h1>Making and editing buttons</h1>
<p><b>New:</b> the <b>Add Button</b> toolbar button, Ctrl+N, or right-click empty
space → <b>Add button here</b>.<br>
<b>Edit:</b> right-click a button → <b>Edit…</b>, or select it and press F2.</p>
<h2>The Edit dialog</h2>
<ul>
<li><b>Label</b> — the text on the button. It wraps and shrinks to fit.</li>
<li><b>Runs in</b> — the <a href="topic:modes">run mode</a>; the line underneath
explains the choice.</li>
<li><b>Command</b> — one or more lines of bash, exactly as you'd type them. Lines
run one after another.</li>
<li><b>🔑 Insert secret</b> — puts <code>${NAME}</code> for a vault secret at the
cursor (correctly, even inside single quotes), so you never paste a token.</li>
<li><b>▶ Test run</b> — runs the command as it is now, without saving.</li>
<li><b>Status light</b> — optional; see <a href="topic:statuslights">Status lights</a>.
<b>Try</b> tests it.</li>
<li><b>Color</b>, <b>Page</b>, <b>Ask before running</b>.</li>
<li><b>Checker</b> — see <a href="topic:checker">The command checker</a>. If it finds a
typed-in secret, <b>🔑 Move to vault</b> appears.</li>
</ul>
<p><b>Save</b> (Enter), <b>Cancel</b> (Escape), <b>Delete</b> (red, editing only).
The dialog remembers its size.</p>
<h2>Other ways to change buttons</h2>
<ul>
<li><b>Duplicate</b> (Ctrl+D, or right-click) — copies of the selected buttons,
ready to edit. Handy for a button per model.</li>
<li><b>Runs in ▸ / Color ▸ / Move to page ▸</b> on the right-click menu change all
selected buttons at once.</li>
<li><b>Delete</b> (the Delete key) asks first; Undo brings buttons back.</li>
</ul>
)HTML");

    add("checker", "editing", "The command checker",
        {QStringLiteral("checker"), QStringLiteral("lint"), QStringLiteral("warning"),
         QStringLiteral("syntax error"), QStringLiteral("placeholder"),
         QStringLiteral("empty variable"), QStringLiteral("ssh lines")},
        R"HTML(
<h1>The command checker</h1>
<p>The box under the command re-reads it as you type and explains anything that
won't work the way it looks. It never changes the command. ✖ = won't run,
⚠ = probably not what you want, ℹ = worth knowing.</p>
<table border="1" cellspacing="0">
<tr><th>Message</th><th>What it means / what to do</th></tr>
<tr><td>Bash can't read this command</td><td>A syntax error (e.g. an unclosed
quote). The line number is given. You can still save, after a question.</td></tr>
<tr><td>A … is typed into the command</td><td>A password or token is in plain text.
Press <b>Move to vault</b>. See <a href="topic:secrets">the vault</a>.</td></tr>
<tr><td>sudo asks for your password…</td><td>Only Terminal window mode can ask for
a password. Switch mode, or write <code>pkexec</code> instead of <code>sudo</code> for
a password pop-up.</td></tr>
<tr><td>… waits for you to type</td><td><code>ssh host</code>, <code>ollama run
model</code>, editors, <code>read</code>… need Terminal window mode.</td></tr>
<tr><td>The lines after ssh run here</td><td>After an interactive <code>ssh</code>
line, the following lines run on <i>this</i> computer once you log out. To run
something on the other computer, put it on the ssh line:
<code>ssh -t user@host 'ollama run qwen3.6:27b'</code>.</td></tr>
<tr><td>Line ends with &amp;</td><td>In a terminal, it stops when the terminal
closes. Use Background mode and drop the <code>&amp;</code>.</td></tr>
<tr><td>Looks like a placeholder</td><td>Text such as <code>youruser</code>,
<code>CHANGEME</code> or <code>&lt;host&gt;</code> was never filled in.</td></tr>
<tr><td>$NAME … will be empty</td><td>A capitalised variable that isn't a saved
secret or a system variable — often a typo.</td></tr>
<tr><td>Inside 'single quotes'</td><td>Bash doesn't fill in <code>$NAME</code> inside
single quotes. Write <code>'"${NAME}"'</code>, which Insert secret does for you.</td></tr>
<tr><td>Keeps running until stopped</td><td>In Output-panel mode it runs until you
press Stop; Background mode may suit it better.</td></tr>
</table>
)HTML");

    // ===================================================================
    add("secrets", "", "The secrets vault",
        {QStringLiteral("secret"), QStringLiteral("token"), QStringLiteral("password"),
         QStringLiteral("api key"), QStringLiteral("vault"), QStringLiteral("$NAME"),
         QStringLiteral("environment variable"), QStringLiteral("rename secret")},
        R"HTML(
<h1>The secrets vault</h1>
<p>Passwords, tokens and keys belong in the vault, not in button commands.
Open it with <b>Tools → Secrets vault…</b>, the <b>🔑 Secrets</b> toolbar button, or
Ctrl+K.</p>
<h2>Using a secret</h2>
<p>Write its name with a <code>$</code> in front, the normal shell way:</p>
<pre>curl -X POST -H "Authorization: Bearer $HA_TOKEN" \
     -H "Content-Type: application/json" \
     -d '{"entity_id": "all"}' \
     http://localhost:8123/api/services/light/turn_on</pre>
<p>Inside <i>single</i> quotes bash won't fill it in, so there write
<code>'"${HA_TOKEN}"'</code> — or let <b>Insert secret</b> in the Edit dialog do it.</p>
<h2>In the vault window</h2>
<ul>
<li><b>Add…</b> — a name (capital letters, digits, _) and the value. Paste the value;
tick <b>Show</b> to check it.</li>
<li><b>Change value…</b> (or double-click) — e.g. after making a new token.</li>
<li><b>Rename…</b> — every button using it is updated to the new name.</li>
<li><b>Delete</b> — tells you which buttons still use it.</li>
<li><b>Show values</b> — values are hidden unless ticked. <b>Used by</b> lists the
buttons that use each secret.</li>
</ul>
<h2>Why this is safer</h2>
<ul>
<li>When a button runs, the secret is handed to the command as an <b>environment
variable</b>. Command lines can be seen by any user on the computer (with
<code>ps</code>); a program's environment only by you and the system administrator.</li>
<li>A command gets only the secrets it names.</li>
<li>Secrets are blanked out (<code>••••(HA_TOKEN)</code>) in the Output panel, logs
shown here, and status-light tooltips.</li>
<li>Buttons can be exported or shown on screen without giving the secret away.</li>
</ul>
<p>The vault file is readable only by you, like your <code>~/.ssh</code> keys. It is
not encrypted with a password — that would mean typing one every time. See
<a href="topic:security">How things are kept safe</a>.</p>
<p>Names like <code>PATH</code>, <code>HOME</code> or anything starting
<code>LD_</code> are refused, because replacing them would change how every program
behaves.</p>
)HTML");

    add("securitycheck", "secrets", "Security check",
        {QStringLiteral("security check"), QStringLiteral("scan"), QStringLiteral("typed-in secret"),
         QStringLiteral("warning badge"), QStringLiteral("old copies"), QStringLiteral("rotate token")},
        R"HTML(
<h1>Security check</h1>
<p><b>Tools → Security check…</b> (also in the vault window, and the yellow
<span class="warn">⚠</span> in the status bar) looks through every button for
passwords and tokens typed into commands — Bearer tokens, web tokens, GitHub and API
keys, passwords in web addresses, <code>sshpass -p</code>, <code>-u user:pass</code>,
<code>password=…</code> and similar.</p>
<h2>What it does</h2>
<ol>
<li>Lists what it found and in which buttons.</li>
<li><b>Move to vault</b> saves each one in the vault (the same token used by several
buttons becomes one secret), and rewrites the commands to use its name, keeping the
quoting correct. One Undo puts the commands back as they were; the secrets stay in
the vault.</li>
<li>It then looks for <b>old copies</b> still holding those values — the backups and
the version-1 <code>buttons.json</code> beside the old program — and offers to clean
them. Cleaning replaces only the secret text with its name; nothing else in those
files changes.</li>
<li>It also makes sure the buttons and vault files are readable only by you.</li>
</ol>
<p class="warn">A token that was ever stored in plain text — or copied anywhere —
should be replaced. Make a new one (in Home Assistant: your profile → Security →
Long-lived access tokens), delete the old one, then use <b>Change value…</b> in the
vault. No button needs editing.</p>
)HTML");

    // ===================================================================
    add("pages", "", "Pages",
        {QStringLiteral("page"), QStringLiteral("tab"), QStringLiteral("rename page"),
         QStringLiteral("new page"), QStringLiteral("delete page"), QStringLiteral("move to page")},
        R"HTML(
<h1>Pages</h1>
<p>Pages are tabs that group buttons — for example <i>AI</i>, <i>Home</i>,
<i>Servers</i>. Your version-1 buttons are all on <i>Main</i>.</p>
<ul>
<li><b>New page:</b> the <b>+</b> at the right of the tabs, File → New page, or
Ctrl+T.</li>
<li><b>Rename:</b> double-click the tab, or right-click it → Rename.</li>
<li><b>Reorder:</b> drag a tab left or right.</li>
<li><b>Delete:</b> right-click the tab → Delete page. If it has buttons you're asked
first. Undo brings it back.</li>
<li><b>Move buttons between pages:</b> select them, right-click → <b>Move to page ▸</b>,
or change <b>Page</b> in the Edit dialog. They're placed below what's already there.</li>
<li><b>Switch pages:</b> click a tab, or Ctrl+Page Down / Ctrl+Page Up.</li>
</ul>
<p>The page you were on is remembered.</p>
)HTML");

    // ===================================================================
    add("arranging", "", "Arranging buttons",
        {QStringLiteral("move"), QStringLiteral("resize"), QStringLiteral("select"),
         QStringLiteral("selection"), QStringLiteral("organize"), QStringLiteral("grid"),
         QStringLiteral("snap"), QStringLiteral("same size"), QStringLiteral("nudge"),
         QStringLiteral("rubber band"), QStringLiteral("drag")},
        R"HTML(
<h1>Arranging buttons</h1>
<h2>Selecting</h2>
<ul>
<li><b>Ctrl+click</b> a button to select it or unselect it; <b>Shift+click</b> adds
it. Selected buttons have a gold border.</li>
<li><b>Drag on empty space</b> to draw a box — every button it touches is selected
(hold Ctrl to add to the selection).</li>
<li><b>Ctrl+A</b> selects every button on the page; <b>Escape</b> or a click on empty
space clears the selection.</li>
</ul>
<h2>Moving and resizing</h2>
<ul>
<li><b>Drag a button</b> to move it. If it's selected, all selected buttons move
together.</li>
<li><b>Drag the grip</b> (bottom-right corner) to resize; all selected buttons grow or
shrink by the same amount.</li>
<li><b>Arrow keys</b> nudge the selection 10 pixels; <b>Shift+arrows</b> 1 pixel.</li>
<li>Moves snap to a 10-pixel grid (switch off in <a href="topic:preferences">Preferences</a>).</li>
</ul>
<p>Each move or resize is one step you can <a href="topic:undo">undo</a>. None of
this works while the layout is <a href="topic:lock">locked</a>.</p>
<h2>Tidying</h2>
<ul>
<li><b>Organize</b> (Ctrl+G) lines the buttons up in rows and columns that fit the
window, in reading order. With two or more selected, only those are arranged,
starting where they are.</li>
<li><b>Make All Same Size</b> (Ctrl+M) copies the size of the button you selected
<i>last</i>: to every button on the page if only one is selected, or to the other
selected buttons if several are.</li>
</ul>
)HTML");

    add("lock", "arranging", "Lock layout",
        {QStringLiteral("lock"), QStringLiteral("unlock"), QStringLiteral("accidental move"),
         QStringLiteral("dot grid")},
        R"HTML(
<h1>Lock layout</h1>
<p><b>🔒 Lock</b> on the toolbar (Arrange → Lock layout, Ctrl+L) stops buttons from
being moved or resized. The button turns green while locked, the dot grid disappears,
and a drag simply does nothing — it never runs the button either.</p>
<p>Clicking, editing, colors and everything else still work. Unlock to rearrange.
The setting is remembered.</p>
)HTML");

    add("undo", "arranging", "Undo and redo",
        {QStringLiteral("undo"), QStringLiteral("redo"), QStringLiteral("mistake"),
         QStringLiteral("ctrl+z")},
        R"HTML(
<h1>Undo and redo</h1>
<p><b>Edit → Undo</b> (Ctrl+Z, or the toolbar) takes back the last change to your
buttons or pages: a move, resize, edit, delete, duplicate, organize, page change,
import, restore, or moving secrets out of commands. The menu says what will be
undone. <b>Redo</b> is Ctrl+Shift+Z or Ctrl+Y.</p>
<p>The last 100 changes can be undone. The history is cleared when the program
closes — for older states, use <a href="topic:backups">backups</a>.</p>
<p>Undo doesn't touch the secrets vault itself.</p>
)HTML");

    // ===================================================================
    add("backups", "", "Backups and restoring",
        {QStringLiteral("backup"), QStringLiteral("restore"), QStringLiteral("damaged file"),
         QStringLiteral("corrupt")},
        R"HTML(
<h1>Backups and restoring</h1>
<p>The first time your buttons are saved in a session, the previous file is copied
into the backups folder. The newest 20 copies are kept. Extra copies are made just
before a restore and before an import.</p>
<p>Saving itself is safe: the new file is written completely before it replaces the
old one, so a crash or a full disk can't leave you with half a file.</p>
<h2>Restoring</h2>
<p><b>File → Restore from backup…</b> lists the copies by date and number of buttons.
Pick one; your current buttons are backed up first, and <b>Undo</b> reverses the
restore.</p>
<h2>If the buttons file is damaged</h2>
<p>If <code>buttons.json</code> can't be read at start-up, it is moved into the backups
folder (renamed <code>…-unreadable-…</code>, never deleted) and you start with an empty
page. Use Restore from backup to bring back the last good copy.</p>
)HTML");

    add("importexport", "backups", "Import and export",
        {QStringLiteral("import"), QStringLiteral("export"), QStringLiteral("share"),
         QStringLiteral("another computer"), QStringLiteral("copy buttons")},
        R"HTML(
<h1>Import and export</h1>
<p><b>File → Export buttons…</b> saves the current page, or all pages, to a file — for
example to set up the same buttons on another computer.</p>
<ul>
<li>Secrets in the vault are <b>not</b> exported, only their names. On the other
computer, add the same names to its vault.</li>
<li>If any command still has a typed-in secret, you're warned before exporting.</li>
</ul>
<p><b>File → Import buttons…</b> adds the pages from such a file as new pages; nothing
you already have is changed. Before anything is added you're shown <b>every label and
command</b> in the file, because importing buttons means trusting their commands. Only
import files you made or trust. Undo removes an import.</p>
<p>Version-1 <code>buttons.json</code> files can be imported too.</p>
)HTML");

    // ===================================================================
    add("launcher", "", "Desktop launcher",
        {QStringLiteral("launcher"), QStringLiteral("dock"), QStringLiteral("favorites"),
         QStringLiteral("applications menu"), QStringLiteral("icon")},
        R"HTML(
<h1>Desktop launcher</h1>
<p><b>File → Create desktop launcher</b> adds Command Button Creator, with its icon, to
your Applications. Then, to keep it in the dock: open Applications, right-click
<i>Command Button Creator</i> → <b>Add to Favorites</b>.</p>
<p>The launcher points at the program where it is now. If you move or rebuild it in
a different folder, create the launcher again — it simply replaces the old one.</p>
<p>Only one copy of the program runs at a time; starting a second one tells you the
first is already open.</p>
)HTML");

    add("zoom", "", "Text size and zoom",
        {QStringLiteral("zoom"), QStringLiteral("text size"), QStringLiteral("font"),
         QStringLiteral("bigger"), QStringLiteral("smaller"), QStringLiteral("ctrl+wheel")},
        R"HTML(
<h1>Text size and zoom</h1>
<p>Hold <span class="key">Ctrl</span> and turn the mouse wheel over the buttons, or use
<b>View → Zoom in / Zoom out / Normal size</b> (Ctrl+= , Ctrl+− , Ctrl+0). The whole
program — menus, dialogs and button labels — grows or shrinks together, from 7 to 24
points. The size is remembered.</p>
<p>The window is kept compact on purpose, so it fits on screen even with large
system text. Button labels shrink a little by themselves when a label is too long for
its button.</p>
<p>The help window has its own <b>A−</b> / <b>A+</b> buttons.</p>
)HTML");

    add("preferences", "", "Preferences",
        {QStringLiteral("preferences"), QStringLiteral("settings"), QStringLiteral("options"),
         QStringLiteral("terminal program"), QStringLiteral("interval")},
        R"HTML(
<h1>Preferences</h1>
<p><b>Tools → Preferences…</b> (Ctrl+,)</p>
<ul>
<li><b>Terminal</b> — the program Terminal-window buttons open. <i>Automatic</i> uses
the first one found (GNOME Terminal on Pop!_OS). Only installed programs are listed.</li>
<li><b>Status lights every</b> — how often <a href="topic:statuslights">status
lights</a> are checked (10 seconds to 10 minutes; normally 30 seconds).</li>
<li><b>Open the Output panel when an Output-panel button runs</b> — on by default.</li>
<li><b>Snap buttons to a 10-pixel grid</b> — on by default.</li>
</ul>
<p>Window size, zoom, the current page, Lock layout and dialog sizes are remembered
automatically.</p>
)HTML");

    // ===================================================================
    add("shortcuts", "", "Keyboard shortcuts",
        {QStringLiteral("keyboard"), QStringLiteral("shortcut"), QStringLiteral("keys"),
         QStringLiteral("hotkey")},
        R"HTML(
<h1>Keyboard shortcuts</h1>
<table border="1" cellspacing="0">
<tr><th>Keys</th><th>Does</th></tr>
<tr><td>F1</td><td>Help</td></tr>
<tr><td>Tab, then Enter or Space</td><td>Move between buttons; run the one with the dotted ring</td></tr>
<tr><td>Menu key</td><td>Right-click menu for the button with the dotted ring</td></tr>
<tr><td>Ctrl+N</td><td>New button</td></tr>
<tr><td>F2</td><td>Edit the selected button</td></tr>
<tr><td>Ctrl+D</td><td>Duplicate selected buttons</td></tr>
<tr><td>Delete</td><td>Delete selected buttons (asks first)</td></tr>
<tr><td>Ctrl+A / Escape</td><td>Select all on this page / clear the selection</td></tr>
<tr><td>Arrow keys / Shift+arrows</td><td>Nudge the selection 10 px / 1 px</td></tr>
<tr><td>Ctrl+Z</td><td>Undo</td></tr>
<tr><td>Ctrl+Shift+Z or Ctrl+Y</td><td>Redo</td></tr>
<tr><td>Ctrl+L</td><td>Lock / unlock the layout</td></tr>
<tr><td>Ctrl+G</td><td>Organize into a grid</td></tr>
<tr><td>Ctrl+M</td><td>Make same size</td></tr>
<tr><td>Ctrl+T</td><td>New page</td></tr>
<tr><td>Ctrl+Page Down / Ctrl+Page Up</td><td>Next / previous page</td></tr>
<tr><td>F9</td><td>Show / hide the Output panel</td></tr>
<tr><td>F5</td><td>Refresh status lights now</td></tr>
<tr><td>Ctrl+K</td><td>Secrets vault</td></tr>
<tr><td>Ctrl+,</td><td>Preferences</td></tr>
<tr><td>Ctrl+wheel, Ctrl+= , Ctrl+− , Ctrl+0</td><td>Zoom in, out, normal</td></tr>
<tr><td>Ctrl+Q</td><td>Quit</td></tr>
<tr><td>Alt+Left / Alt+Right</td><td>Back / forward in this help</td></tr>
</table>
)HTML");

    // ===================================================================
    add("recipes", "", "Handy command recipes",
        {QStringLiteral("examples"), QStringLiteral("recipes"), QStringLiteral("home assistant"),
         QStringLiteral("ssh"), QStringLiteral("ollama"), QStringLiteral("docker"),
         QStringLiteral("systemctl"), QStringLiteral("pkexec")},
        R"HTML(
<h1>Handy command recipes</h1>
<h2>Home Assistant (Output panel mode)</h2>
<pre>curl -sf -X POST -H "Authorization: Bearer $HA_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"entity_id": "script.blackout"}' \
  http://localhost:8123/api/services/script/turn_on</pre>
<p><code>-sf</code> makes curl fail (✗ on the button) if Home Assistant refuses.</p>
<h2>Run a model on another computer (Terminal window mode)</h2>
<pre>ssh -t mark@100.83.36.84 'ollama run qwen3.6:27b'</pre>
<p>The command goes on the ssh line; <code>-t</code> gives it a terminal to chat in.</p>
<h2>A remote reading (Output panel mode)</h2>
<pre>ssh mark@100.83.36.84 nvidia-smi --query-gpu=name,temperature.gpu --format=csv</pre>
<h2>Restart things (Output panel mode)</h2>
<pre>docker restart homeassistant
systemctl --user restart syncthing.service</pre>
<h2>A server that should keep running (Background mode)</h2>
<pre>cd ~/open-webui
source .venv/bin/activate
DATA_DIR="$HOME/open-webui/data" OLLAMA_BASE_URL="http://127.0.0.1:11434" \
  open-webui serve --host 127.0.0.1 --port 3000</pre>
<p>Status light: <code>curl -sf -o /dev/null http://127.0.0.1:3000</code></p>
<h2>Restart Ollama</h2>
<p>If Ollama runs as a system service (the usual Linux install), in Terminal window
mode:</p>
<pre>sudo systemctl restart ollama</pre>
<p>Or with a password pop-up instead of a terminal (Output panel mode):</p>
<pre>pkexec systemctl restart ollama</pre>
<p>If you start Ollama by hand instead, use Background mode with no <code>&amp;</code>:</p>
<pre>pkill -x ollama; sleep 2; ollama serve</pre>
)HTML");

    // ===================================================================
    add("files", "", "Where files are kept",
        {QStringLiteral("files"), QStringLiteral("folder"), QStringLiteral("location"),
         QStringLiteral("buttons.json"), QStringLiteral("secrets.json"), QStringLiteral("config")},
        R"HTML(
<h1>Where files are kept</h1>
<table border="1" cellspacing="0">
<tr><th>What</th><th>Where</th></tr>
<tr><td>Your buttons and pages</td><td><code>~/.config/MarkCo/CommandButtonCreator/buttons.json</code></td></tr>
<tr><td>The secrets vault</td><td><code>~/.config/MarkCo/CommandButtonCreator/secrets.json</code></td></tr>
<tr><td>Backups</td><td><code>~/.config/MarkCo/CommandButtonCreator/backups/</code></td></tr>
<tr><td>Background-mode logs</td><td><code>~/.local/share/MarkCo/CommandButtonCreator/logs/</code></td></tr>
<tr><td>Window size, zoom, preferences</td><td><code>~/.config/MarkCo/CommandButtonCreator.conf</code></td></tr>
<tr><td>Desktop launcher</td><td><code>~/.local/share/applications/commandbuttoncreator.desktop</code></td></tr>
</table>
<p>The folders can be opened only by you, and the buttons, vault, backups and logs
are readable only by you. Nothing is stored anywhere else, and nothing is sent
anywhere.</p>
<p>Version 1 kept <code>buttons.json</code> next to the program in the build folder.
That file was copied once and is never changed by 2.0 (except by the Security check's
cleaning, if you agree to it).</p>
)HTML");

    add("security", "files", "How things are kept safe",
        {QStringLiteral("security"), QStringLiteral("privacy"), QStringLiteral("permissions"),
         QStringLiteral("safe"), QStringLiteral("offline")},
        R"HTML(
<h1>How things are kept safe</h1>
<ul>
<li><b>Nothing leaves this computer.</b> The program has no network code of its own;
only your commands reach the network, when you run them. The help never opens web
pages.</li>
<li><b>Private files.</b> Folders are 0700 and files 0600 (only you), checked and
corrected at start-up and by the Security check.</li>
<li><b>Secrets stay out of commands</b>, are passed to only the commands that name
them, through the environment rather than the command line, and are blanked out of
everything shown. See <a href="topic:secrets">the vault</a>.</li>
<li><b>Commands are passed whole.</b> A command is handed to bash as a single piece,
never glued into another command, so nothing in it can escape its quotes.</li>
<li><b>Nothing to type into.</b> Output-panel, Background and status-check commands get
no keyboard input, so they can't hang waiting for a password.</li>
<li><b>Careful reading.</b> The buttons file is checked field by field when loaded —
odd values are corrected, sizes and lengths capped — so a damaged or edited file
can't crash the program. A damaged file is set aside, never overwritten.</li>
<li><b>Safe saving</b> (complete file or nothing), automatic
<a href="topic:backups">backups</a>, and <a href="topic:undo">undo</a>.</li>
<li><b>Imports are shown before they're added</b>, with every command listed.</li>
<li><b>One copy at a time</b>, so two windows can't overwrite each other's changes.</li>
</ul>
<p>What it can't do: protect against someone who is logged in as you, or the system
administrator — they can read your files whatever any program does.</p>
)HTML");

    // ===================================================================
    add("troubleshooting", "", "Troubleshooting",
        {QStringLiteral("problem"), QStringLiteral("not working"), QStringLiteral("error"),
         QStringLiteral("red cross"), QStringLiteral("nothing happens"), QStringLiteral("help")},
        R"HTML(
<h1>Troubleshooting</h1>
<h2>A button shows a red ✗</h2>
<p>Open the Output panel (F9) and click the run — the message usually says why.
Exit code 127 means a program wasn't found (check the spelling, or use the full
path); 126 means it isn't allowed to run; 1 is a general failure.</p>
<h2>Nothing seems to happen</h2>
<ul>
<li>Is it in <b>Background</b> mode? Then it's running quietly — right-click → Show
log.</li>
<li>Is <b>Ask before running</b> on, with the question hidden behind another
window?</li>
</ul>
<h2>“No terminal program was found”</h2>
<p>Pick one in <a href="topic:preferences">Preferences</a>, or install GNOME Terminal.</p>
<h2>A command works in a terminal but not in Output-panel mode</h2>
<ul>
<li>It asks a question or a password — use Terminal window mode (the
<a href="topic:checker">checker</a> usually warns about this).</li>
<li>It relies on something your terminal sets up (an alias, or <code>nvm</code>). Use
the full path to the program, as in
<code>/home/markmandy/.nvm/versions/node/v18.20.8/bin/node</code>.</li>
<li>Commands start in your home folder; add a <code>cd</code> line if they need
another.</li>
</ul>
<h2>A status light is always red</h2>
<p>Press <b>Try</b> beside it in the Edit dialog. Status checks can't use
<code>sudo</code>, and are stopped after 8 seconds.</p>
<h2>The buttons won't move</h2>
<p>The layout is <a href="topic:lock">locked</a> — press 🔒 Lock (Ctrl+L).</p>
<h2>I deleted or broke something</h2>
<p>Ctrl+Z (<a href="topic:undo">Undo</a>), or File → Restore from backup.</p>
)HTML");

    add("about", "", "About",
        {QStringLiteral("about"), QStringLiteral("version"), QStringLiteral("author")},
        R"HTML(
<h1>About Command Button Creator</h1>
<p>Version 2.0 — Tardis Software.</p>
<p>Created by Mark Mcdougal, March 5, 2026. Version 2.0, November 2026.</p>
<p>Built with Qt. Runs entirely on this computer: no accounts, no servers, no
network access of its own.</p>
)HTML");

    return t;
}

} // namespace

const QList<HelpTopic> &topics()
{
    static const QList<HelpTopic> all = build();
    return all;
}

const HelpTopic *find(const QString &id)
{
    for (const HelpTopic &t : topics())
        if (t.id == id)
            return &t;
    return nullptr;
}

} // namespace HelpContent
