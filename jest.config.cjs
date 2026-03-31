/** @type {import('jest').Config} */
module.exports = {
  transform: {
    '^.+\\.tsx?$': ['ts-jest', { tsconfig: { module: 'commonjs' } }],
    '^.+\\.jsx?$': 'babel-jest',
  },
  testEnvironment: 'node',
};
