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

let timer = null, running = false, again = false;
function flush() {
  if (running) { again = true; return; }
  running = true;
  FS.syncfs(false, function (err) {                // false = memória -> IndexedDB
    if (err) console.error('IDBFS save', err);
    running = false;
    if (again) { again = false; flush(); }
  });
}
Module.persist = function () {                     // com debounce de 1,5 s
  clearTimeout(timer);
  timer = setTimeout(flush, 1500);
};
addEventListener('pagehide', flush);
addEventListener('visibilitychange', () => { if (document.hidden) flush(); });