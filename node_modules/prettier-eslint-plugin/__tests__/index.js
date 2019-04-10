import path from 'path'
import * as PluginFile from './../'

describe('PrettierEslintPlugin', () => {
  const plugin = new PluginFile.PrettierEslintPlugin({
    encoding: 'utf-16',
    extensions: ['.coffee', '.ts'],
    // Prettier ESlint API
    filePath: '/file/path/',
    eslintConfig: {
      parserOptions: {
        ecmaVersion: 7
      },
      rules: {
        semi: ['error', 'never']
      }
    },
    prettierOptions: {
      bracketSpacing: true,
    },
    logLevel: 'trace',
    eslintPath: '/eslint/path/',
    prettierPath: '/prettier/path/'
  })
  it('supports other encoding than utf-8', () => {
    expect(plugin.encoding).toBe('utf-16')
  })
  it('supports different file extensions', () => {
    expect(plugin.extensions).toEqual(['.coffee', '.ts'])
  })
  it('exposes Prettier ESlint API', () => {
    expect(plugin.filePath).toBe('/file/path/')
    expect(plugin.eslintConfig).toEqual({
      parserOptions: {
        ecmaVersion: 7
      },
      rules: {
        semi: ['error', 'never']
      }
    })
    expect(plugin.prettierOptions).toEqual({
      bracketSpacing: true,
    })
    expect(plugin.logLevel).toBe('trace')
    expect(plugin.eslintPath).toBe('/eslint/path/')
    expect(plugin.prettierPath).toBe('/prettier/path/')
  })
})
