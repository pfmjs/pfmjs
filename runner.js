const path = require('path');
const fs = require('fs');
const vm = require('vm');

const scriptPath = process.argv[2];

if (!scriptPath) {
  console.error('Usage: node runner.js <file>');
  process.exit(1);
}

// Read the module code from its relative path
function readModule(modulePath) {
  try {
    return fs.readFileSync(modulePath, 'utf-8');
  } catch (err) {
    console.error('Error reading module:', err);
    return null;
  }
}

// Replace 'import' with module content based on the path
function replaceImportStatements(userScript) {
  return userScript.replace(/import\s+(.+?)\s+from\s+['"](.+?)['"]/g, (match, variable, modulePath) => {
    // Resolve the full path to the module
    const fullPath = path.resolve(__dirname, modulePath);

    const moduleCode = readModule(fullPath); // Read the module code dynamically
    if (!moduleCode) {
      console.error(`Module not found: ${fullPath}`);
      return match; // Return the original import if module not found
    }

    // Replace the import statement with the actual module code
    return `
      const ${variable} = ${moduleCode}; // Injected module
    `;
  });
}

// Run the combined code in a VM context
async function runScript(scriptPath) {
  try {
    const userScript = fs.readFileSync(scriptPath, 'utf-8'); // Read the user's script

    const modifiedScript = replaceImportStatements(userScript); // Replace imports

    const context = vm.createContext({
      console, // Add console for logging
      module: {}, // Add module object if needed
      exports: {}, // Add exports for module
    });

    const script = new vm.Script(modifiedScript);
    script.runInContext(context);
    console.log()
  } catch (err) {
    console.error('Error loading script:', err);
  }
}

runScript(scriptPath);
