import fs from 'fs'

export default function (filename, data, options) {
  return new Promise(function (resolve, reject) {
    fs.writeFile(filename, data, options, (err) => err === null ? resolve(filename) : reject(err))
  })
}
