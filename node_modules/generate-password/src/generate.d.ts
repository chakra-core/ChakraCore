interface Options {
  length?: number
  numbers?: boolean
  symbols?: boolean
  uppercase?: boolean
  excludeSimilarCharacters?: boolean
  exclude?: string
  strict?: boolean
}
export function generate (options?: Options): string
export function generateMultiple (amount: number, options?: Options): string[]
