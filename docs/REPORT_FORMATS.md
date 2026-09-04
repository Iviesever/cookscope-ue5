# Report Formats

One immutable Snapshot/RuleConfig/Analysis/Diff input produces four projections:

- JSON: canonical `cookscope.result/1`, the semantic authority.
- SARIF 2.1.0: stable Rule IDs, levels, messages, help URIs, and asset locations.
- JUnit XML: one deterministic testcase per configured rule; findings below FailThreshold remain passing output, blocking findings become failures, and analysis diagnostics become errors.
- HTML: one self-contained offline file with embedded canonical JSON, CSS, and JavaScript.

The HTML report offers overview cards; Severity, Rule, Class, Path, Chunk, Bundle, and baseline-state filters; size/delta sorting; Baseline/Candidate summaries; finding dependency chains; and an asset/Chunk/Bundle table. It has no CDN, external font, script, or stylesheet. Embedded `<` and `&` values are neutralized before insertion into the JSON script element, and all dynamic markup is escaped.

Browser verification covers real interactions, desktop and narrow rendering, and an empty Console warning/error list. Reproduce screenshots with `scripts/Capture-HtmlReport.ps1`.
