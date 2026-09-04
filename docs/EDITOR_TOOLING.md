# Editor Tooling

The CookScope Nomad Tab is registered by `CookScopeEditor` and hosts `SCookScopePanel`.

The panel supports scope/config/source SHA/baseline/Cook registry inputs, Run/Cancel/Export actions, progress/status, Severity/Rule/Class/Path filters, selectable finding expansion, Baseline/Candidate and size-change summaries, typed dependency details, why-cooked roots/target, Content Browser sync, asset opening, and JSON/SARIF/JUnit/HTML export.

`FCookScopeEditorSession` contains no rule implementation. It performs bounded Asset Registry acquisition for the chosen scope on the permitted Editor thread, then runs Core diff/rule/graph/report computation on a generation-owned worker future. Cancel invalidates the generation. Shutdown cancels, joins the worker, clears results, closes the live tab, and unregisters the spawner.

The real screenshot is generated from the production widget with `scripts/Capture-EditorScreenshot.ps1`; PACT Automation also asserts the tab hosts `SCookScopePanel` rather than a placeholder.
