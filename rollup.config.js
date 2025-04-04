import commonjs from '@rollup/plugin-commonjs';

export default {
  input: 'build/Release/index.js', // Your entry file
  output: [
    {
      file: 'build/Release/index.cjs', // CommonJS format
      format: 'cjs',
      sourcemap: true,
    },
  ],
  plugins: [commonjs()],
  external: [
    'fs',
    'http',
    'https',
    'crypto',
    'module'
  ] // Exclude Node.js built-in modules from the bundle
};