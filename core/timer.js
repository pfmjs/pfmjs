module.exports = {
    sleep(ms) {
      console.log(`Sleeping for ${ms}ms...`);
      const start = Date.now();
      while (Date.now() - start < ms) {}
    }
  };  