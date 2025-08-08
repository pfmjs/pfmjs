// built-in fs module
import * as fs from "../modules/fs.mts";

// ./tests is added because this file will tested from the project root
// writes file using built-in fs
fs.writeFile('./tests/helloWorld.txt', 'hi from helloWorld.txt')

const fileContent = fs.readFile('./tests/helloWorld.txt') 
console.log(fileContent)