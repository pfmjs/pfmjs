import * as fs from "../modules/fs.mjs"
import * as std from "std";

fs.writeFile('./tests/helloWorld.txt', 'hi from helloWorld.txt')

const fileContent = fs.readFile('./tests/helloWorld.txt')

console.log(fileContent)