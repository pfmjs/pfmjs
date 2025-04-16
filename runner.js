const path = require('path');
const { loadModule } = require('./loader');

// Load user app
const scriptPath = process.argv[2];
if (!scriptPath) {
  console.error('Usage: node runner.js <file>');
  process.exit(1);
}

loadModule(path.resolve(scriptPath));
