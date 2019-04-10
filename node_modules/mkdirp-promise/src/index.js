import mkdirp from 'mkdirp'

export default function (dir, opts) {
  return new Promise((resolve, reject) => {
    mkdirp(dir, opts, (err, made) => err === null ? resolve(made) : reject(err))
  })
}
