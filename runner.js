const path = require('path');
const { loadESModule } = require('./loader');

// Load user app
const scriptPath = process.argv[2];
if (!scriptPath) {
  console.error('Usage: node runner.js <file>');
  process.exit(1);
}

async function runScript(scriptPath) {
  try {
    const moduleExports = await loadESModule(scriptPath);
    console.log('Module executed:', moduleExports);
  } catch (err) {
    console.error('Error loading script:', err);
  }
}

runScript(path.resolve(scriptPath));
