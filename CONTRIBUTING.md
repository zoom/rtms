# Contributing to Zoom RTMS SDK

Thank you for your interest in contributing to the Zoom RTMS SDK! This document provides guidelines and instructions for contributing to this project.

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [Development Setup](#development-setup)
  - [Prerequisites](#prerequisites)
  - [Getting Started](#getting-started)
- [Build and Test](#build-and-test)
- [Pull Request Process](#pull-request-process)
- [Coding Standards](#coding-standards)
- [Documentation](#documentation)
- [Issue Reporting](#issue-reporting)
- [License](#license)

## Code of Conduct

This project adheres to the Zoom Open Source Code of Conduct. By participating, you are expected to uphold this code.

## Development Setup

### Prerequisites

- **Node.js**: v20.3.0+ (LTS versions recommended: 20.x or 22.x)
- **Python**: v3.10 or later
- **CMake**: v3.25 or later
- **C/C++ Build Tools**:
  - Linux: GCC 9+ and make
  - macOS: Xcode Command Line Tools

### Getting Started

1. **Fork the repository** on GitHub
2. **Clone your fork**:
   ```bash
   git clone https://github.com/YOUR-USERNAME/rtms.git
   cd rtms
   ```
3. **Set up the upstream remote**:
   ```bash
   git remote add upstream https://github.com/zoom/rtms.git
   ```
4. **Install dependencies**:
   ```bash
   npm install
   ```
5. **Copy the environment template**:
   ```bash
   cp .env.example .env
   ```
   Then edit the `.env` file with your Zoom developer credentials.

## Build and Test

The project provides several npm scripts for common development tasks:

```bash
# Building modules
npm run build         # Build all modules
npm run build:js      # Build only Node.js module
npm run build:py      # Build only Python module

# Testing
npm run test          # Run all tests
npm run test:js       # Run Node.js tests
npm run test:py       # Run Python tests

# Build modes
npm run debug         # Switch to debug mode
npm run release       # Switch to release mode (default)
```

Before submitting a PR, ensure that:
1. All tests pass
2. Your code builds without warnings
3. Your code is properly documented

## Pull Request Process

1. **Create a branch** for your feature or bugfix:
   ```bash
   git checkout -b feature/your-feature-name
   # or
   git checkout -b fix/your-bugfix-name
   ```

2. **Make your changes** and commit them using meaningful commit messages:
   ```bash
   git commit -m "feat: Add support for new feature"
   ```
   Follow [Conventional Commits](https://www.conventionalcommits.org/) for commit messages.

3. **Push your branch** to your fork:
   ```bash
   git push origin feature/your-feature-name
   ```

4. **Open a Pull Request** against the main branch of the upstream repository.

5. **PR Requirements**:
   - Include a clear description of the changes
   - Update documentation as needed
   - Add or update tests as appropriate
   - Ensure CI passes (build, tests, linting)
   - Obtain at least one approval from a maintainer

## Coding Standards

- **JavaScript/TypeScript**:
  - Follow the project's ESLint configuration
  - Use TypeScript for new code when possible
  - Maintain consistent error handling patterns

- **Python**:
  - Follow PEP 8 style guide
  - Support Python 3.10+
  - Use type annotations where appropriate

- **C++**:
  - Follow the project's existing C++ style
  - Ensure memory safety and proper resource management
  - Maintain ABI compatibility

## Documentation

- Update the README.md when adding significant features
- Add JSDoc/docstring comments to all public APIs
- For complex features, consider adding examples to the docs folder

## Issue Reporting

- Use the bug report template for bugs
- Use the feature request template for new features
- Provide as much detail as possible

## License

By contributing to this project, you agree that your contributions will be licensed under the project's [MIT License](LICENSE.md).