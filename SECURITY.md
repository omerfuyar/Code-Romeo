# Security Policy

## Reporting a Vulnerability

If you discover a security vulnerability in Code-Romeo, please report it responsibly.

### How to Report

**Do NOT open a public GitHub issue for security vulnerabilities.**

Instead, please:
1. Email the maintainer with details about the vulnerability
2. Include steps to reproduce the issue
3. Describe the potential impact
4. If possible, suggest a fix

You can find contact information in the repository owner's GitHub profile.

### What to Include

When reporting a vulnerability, please include:
- Description of the vulnerability
- Steps to reproduce
- Potential impact
- Affected versions (if known)
- Suggested fix (if you have one)

## Response Timeline

- **Acknowledgment**: Within 48 hours of report
- **Initial Assessment**: Within 1 week
- **Fix Development**: Depends on severity and complexity
- **Public Disclosure**: After fix is available and tested

## Supported Versions

Currently, security updates are provided for:
- `main` branch (latest development version)

Once versioned releases are available, this section will be updated with specific version support timelines.

## Security Measures

Code-Romeo implements several security practices:

### Safe C Practices
- Uses safe string functions (`snprintf`, not `sprintf`)
- Employs bounds checking in debug builds
- Avoids unsafe functions like `strcpy`, `strcat`, `gets`
- Uses assertion macros to validate inputs

### Debug Builds
- AddressSanitizer (ASAN) support for detecting memory errors
- UndefinedBehaviorSanitizer (UBSAN) support
- Extensive debug logging and assertions
- Stack protection with `-fstack-protector-strong`

### Code Quality
- Strict compiler warnings enabled (`-Wall -Wextra -Wpedantic`)
- Static analysis integration (via CI/CD)
- Memory safety checks with sanitizers

### Known Limitations

- **Fixed-size buffers**: Some operations use fixed-size buffers (128 bytes). While bounds-checked, extremely long paths or strings may trigger assertions or truncation.
- **File path validation**: Limited validation of user-provided file paths. Applications using this framework should validate paths before passing to framework functions.
- **Debug logging**: Debug logs may contain sensitive information. Disable in production or ensure log files are properly secured.

## Best Practices for Using Code-Romeo

When using Code-Romeo in your application:

1. **Validate inputs**: Always validate user inputs before passing to framework functions
2. **Path security**: Be cautious with user-provided file paths; validate and sanitize them
3. **Debug builds**: Use debug builds during development to catch issues early
4. **Release builds**: Use release builds in production with appropriate security settings
5. **Log security**: Ensure debug logs are secured or disabled in production
6. **Memory management**: Follow the framework's memory ownership patterns
7. **Update regularly**: Keep your copy of Code-Romeo updated with latest security fixes

## Security Updates

Security updates will be released as soon as possible after a vulnerability is confirmed and fixed.

Updates will be announced through:
- GitHub Security Advisories
- Release notes
- Commit messages with `[SECURITY]` tag

## Out of Scope

The following are generally considered out of scope for security issues:

- Issues in example code (unless they demonstrate vulnerable framework patterns)
- Issues in third-party dependencies (report to respective projects)
- Denial of service through resource exhaustion (framework is for trusted environments)
- Issues requiring physical access to the system

## Attribution

We appreciate responsible disclosure and will acknowledge security researchers who report valid vulnerabilities (unless they prefer to remain anonymous).

## Questions

If you have questions about security that don't involve reporting a vulnerability, please:
- Open a regular GitHub issue
- Use the discussions feature
- Contact the maintainer

---

**Last Updated**: December 11, 2025
