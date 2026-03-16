# CLAUDE.md - AI Assistant Guidelines

## Code Quality Principles

- Verify information before presenting it
- Make changes file-by-file
- No apologies, understanding feedback, or summaries
- Don't suggest whitespace changes or unnecessary updates
- Preserve existing code and structures
- Provide all edits in a single chunk per file
- Always link to real files, not examples

## C++ Standards

### Naming Conventions
- **PascalCase**: Classes, structures
- **camelCase**: Variables, functions, methods
- **ALL_CAPS**: Constants, macros
- **snake_case**: Files, directories

### Code Structure
- English for all code and documentation
- Doxygen-style comments for public APIs
- Functions <20 lines with single purpose
- Classes <200 lines, <10 public methods
- No blank lines within functions
- Use const-correctness throughout

### Best Practices
- Prefer smart pointers over raw pointers (std::unique_ptr, std::shared_ptr)
- Use RAII principles for resource management
- Follow SOLID principles, prefer composition over inheritance
- Use std::optional for possibly null values
- Standard library algorithms over manual loops
- Use std::expected or error codes for expected failures, exceptions for unexpected errors

### Project Organization
```
include/  - Header files
src/      - Source files
test/     - Test files
lib/      - Libraries
doc/      - Documentation
```

## Git Workflow (Gitflow)

### Branch Structure
- **main**: Production-ready code only
- **develop**: Main development branch
- **feature/[issue-id]-description**: From develop, merge back to develop
- **release/vX.Y.Z**: From develop, merge to main + develop
- **hotfix/vX.Y.Z**: From main, merge to main + develop

### Commit Format
```
type(scope): description

Types: feat, fix, docs, style, refactor, test, chore
```

### Pull Request Rules
- Minimum 1 approval required
- CI checks must pass
- Branch must be up-to-date before merge
- Delete branch after merge
- No direct commits to main/develop

### Versioning
Semantic Versioning: MAJOR.MINOR.PATCH
- MAJOR: Incompatible API changes
- MINOR: Backwards-compatible functionality
- PATCH: Backwards-compatible bug fixes

## Clean Code

### Core Principles
- Replace magic numbers with named constants
- Meaningful names that reveal purpose
- Single Responsibility: one function, one purpose
- DRY: Extract repeated code
- Encapsulation: Hide implementation, expose clear interfaces
- Self-documenting code; comments explain "why", not "what"

### Testing
- Write tests when implementing features
- Write tests before fixing bugs
- **Always add a regression test when fixing a bug** to prevent reintroduction
- Follow Arrange-Act-Assert convention
- Test edge cases and error conditions
- Use test doubles for dependencies (except inexpensive third-party libs)
