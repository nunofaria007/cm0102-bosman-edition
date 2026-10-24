Module.preRun = Module.preRun || [];
Module.preRun.push(function () {
  const dir = '/home/web_user/.local/share/cm0102';
  FS.mkdirTree(dir);
  FS.mount(IDBFS, {}, dir);
  addRunDependency('idbfs');                       // o jogo só arranca depois de carregar
  FS.syncfs(true, function (err) {                 // true = IndexedDB -> memória
    if (err) console.error('IDBFS load', err);
    removeRunDependency('idbfs');
  });
});

let timer = null, running = false, again = false, waiting = [], closed = false;
function flush(done) {
  if (done) waiting.push(done);
  if (running) { again = true; return; }
  running = true;
  FS.syncfs(false, function (err) {                // false = memória -> IndexedDB
    if (err) console.error('IDBFS save', err);
    running = false;
    if (again) { again = false; flush(); return; }
    const cbs = waiting; waiting = [];
    cbs.forEach(cb => cb());
  });
}
// O jogo chama isto a cada operação de ficheiro (milhares por segundo na criação de um jogo): o perfil
// mostrou 7,7 % do tempo em clearTimeout/setTimeout. Agora só marca; um único temporizador grava 1,5 s
// depois da primeira alteração e volta a armar-se se houve mais alterações entretanto.
let dirty = false;
function arm() {
  timer = setTimeout(() => {
    timer = null;
    if (!dirty) return;
    dirty = false;
    flush(() => { if (dirty && !timer && !closed) arm(); });
  }, 1500);
}
Module.persist = function () {
  if (closed) return;
  dirty = true;
  if (!timer) arm();
};
const onVisibility = () => { if (document.hidden && !closed) flush(); };
// The page is being left. Save, then wake the game's wait (SDL_Delay in user32.c) and tell it to exit, so
// that its suspended call stack ends. A suspended stack is a GC root: without this the browser keeps
// the whole old game (WebAssembly memory included) alive after a reload.
const onHide = (e) => {
  if (closed) return;
  flush();
  if (e && e.persisted) return;                    // bfcache: the page may come back
  Module.closing = true;
  if (Module.wake) Module.wake();
};
addEventListener('pagehide', onHide);
addEventListener('visibilitychange', onVisibility);

// Chamado pelo jogo quando o utilizador sai (ExitProcess, TerminateProcess ou fim do ponto de entrada).
// Grava tudo no IndexedDB, para de reagir à página e entrega o ecrã a Module.onClosed (shell.html).
Module.onGameExit = function (code) {
  if (closed) return;
  closed = true;
  clearTimeout(timer);
  removeEventListener('pagehide', onHide);
  removeEventListener('visibilitychange', onVisibility);
  flush(function () {
    try { if (Module.canvas) { Module.canvas.width = 1; Module.canvas.height = 1; } } catch (e) {}
    if (Module.onClosed) Module.onClosed(code);
  });
};
