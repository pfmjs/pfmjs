const fs = require('fs');
const vm = require('vm');
const path = require('path');

const customModules = {
  fs: require('./core/fs'),
  http: require('./core/http'),
  timer: require('./core/timer'),
};

function loadModule(filePath) {
  const code = fs.readFileSync(filePath, 'utf-8');

  const dirname = path.dirname(filePath);
  const module = { exports: {} };

  const wrapper = `(function(require, module, exports, __dirname, __filename) {
    ${code}
  })`;

  const script = new vm.Script(wrapper, { filename: filePath });
  const context = vm.createContext({});

  const func = script.runInContext(context);
  func((mod) => {
    if (customModules[mod]) return customModules[mod];
    return require(mod);
  }, module, module.exports, dirname, filePath);

  return module.exports;
}

module.exports = { loadModule };
