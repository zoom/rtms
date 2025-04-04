import typescript from 'rollup-plugin-typescript2';

export default {
  input: 'index.ts', // Your entry file
  output: [
    {
      file: 'build/Release/index.cjs.js', // CommonJS format
      format: 'cjs',
      exports: 'named',
      sourcemap: true,
    },
    {
      file: 'build/Release/index.js', // ES module format
      format: 'es',
      sourcemap: true,
    }
  ],
  plugins: [
    typescript() // Use the TypeScript plugin
  ],
  external: [
    'fs',
    'http',
    'https',
    'crypto',
    'module'
  ] // Exclude Node.js built-in modules from the bundle
};