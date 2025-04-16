const path = require('path');
const fs = require('fs');
const vm = require('vm');

const scriptPath = process.argv[2];

if (!scriptPath) {
  console.error('Usage: pfmjs <file>');
  process.exit(1);
}

// Read the module code from its relative path
function readModule(modulePath) {
  try {
    const resolved = require.resolve(modulePath);
    return fs.readFileSync(resolved, 'utf-8');
  } catch (err) {
    console.error('Error reading module:', err);
    return null;
  }
}

// Replace 'import fs from "./core/fs.js"' with "const fs = require('./core/fs.js');"
function replaceImportStatements(userScript) {
  return userScript.replace(/import\s+(\w+)\s+from\s+['"](.+?)['"]/g, (match, variable, modulePath) => {
    return `const ${variable} = require('${modulePath}');`;
  });
}

// Regex to match all the different import/require styles for 'fs'
const fsRegex = /import\s*(['"]fs['"]);|import\s*\*\s*from\s*['"]fs['"];|const\s+fs\s*=\s*require\(['"]fs['"]\);|import\s*['"]fs['"];/g;

// Run the final script in a VM context
function runScript(scriptPath) {
  try {
    let userScript = fs.readFileSync(scriptPath, 'utf-8');
    
    // Check for all the 'fs' import styles and replace them
    if (fsRegex.test(userScript)) {
      userScript = userScript.replace(fsRegex, "import * from 'fs';");
    }

    // Replace other import statements with 'require'
    const modifiedScript = replaceImportStatements(userScript);

    const context = vm.createContext({
      console,
      require,
      module: {},
      exports: {},
    });

    const script = new vm.Script(modifiedScript);
    script.runInContext(context);
  } catch (err) {
    console.error('Error loading script:', err);
  }
}

runScript(scriptPath);
