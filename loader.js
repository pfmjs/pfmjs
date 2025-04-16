const fs = require('fs');
const path = require('path');
const vm = require('vm');

const customModules = {
  fs: require('./core/fs'),
  http: require('./core/http'),
  timer: require('./core/timer'),
};

async function loadModule(filePath, imports = {}) {
  const code = fs.readFileSync(filePath, 'utf-8');
  const dirname = path.dirname(filePath);
  const module = { exports: {} };

  const wrapper = `
    (function(require, module, exports, __dirname, __filename) {
      ${code}
    })
  `;

  const script = new vm.Script(wrapper, { filename: filePath });
  const context = vm.createContext({});

  // Create a function for import/export handling
  const func = script.runInContext(context);

  // Handling ES6 imports/exports
  const moduleLoader = (mod) => {
    if (customModules[mod]) return customModules[mod];
    if (imports[mod]) return imports[mod];
    return require(mod);
  };

  // Execute the script
  func(moduleLoader, module, module.exports, dirname, filePath);

  return module.exports;
}

async function loadESModule(filePath) {
  const ext = path.extname(filePath);
  if (ext !== '.js') {
    throw new Error('Only .js files are supported.');
  }

  let moduleExports = {};

  // Load the module and handle imports
  const code = fs.readFileSync(filePath, 'utf-8');
  const imports = {};
  const importRegex = /import\s+([\w\d]+)\s+from\s+['"](.+)['"]/g;
  let match;

  while ((match = importRegex.exec(code))) {
    const [_, varName, importPath] = match;
    const resolvedPath = path.resolve(path.dirname(filePath), importPath);
    const importedModule = await loadModule(resolvedPath, imports);
    imports[varName] = importedModule;
  }

  moduleExports = await loadModule(filePath, imports);

  return moduleExports;
}

module.exports = { loadModule, loadESModule };