# ase-markdown

**Design:** DSGN_065 (Metagaming-Web-Ökosystem), DSGN_066 (Browser-App)

[![Layer](https://img.shields.io/badge/Layer-1%20Core-blue.svg)]()
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)]()

> CommonMark/GFM + ASE DSL Markdown parser producing typed AST

Part of [ASE - Antares Simulation Engine](../../..)

## Overview

The **ase-markdown** module parses Markdown files into a typed Abstract Syntax Tree (AST). It supports standard CommonMark and GitHub Flavored Markdown via cmark-gfm, extended with ASE-specific syntax:

- **Callout boxes:** `> [!INFO]`, `> [!WARNING]`, `> [!TIP]`, `> [!NOTE]`
- **KaTeX math:** `$inline$` and `$$display$$`
- **NerdFont icons:** `(nf-fa-name)` pattern detection
- **Code block classification:** `mermaid`, `diff`, `svgbob`, `ase-math` language tags
- **YAML frontmatter:** title, description, version, keywords, curated flag
- **CMS DSL directives:** 21 block (`:::name{attrs}`), 10 leaf (`::name{attrs}`), 7 inline (`[[wiki]]`, `{{glossary}}`, `{ref:}`, `{icon:}`, `{version}`, `:tip[]{}`)

## Modes

- **TECH mode:** CommonMark + GFM + callouts + math + icons + code classification + frontmatter
- **DSGN mode:** Everything from TECH + full DSL directive parsing (37 directives)

## Build

```bash
cmake -B build -G Ninja && ninja -C build
ninja -C build ase-markdown-test   # Run tests
```

## Dependencies

- **cmark-gfm** v0.29.0.gfm.13 (FetchContent)
- **doctest** v2.4.12 (FetchContent, tests only)
- **ase-types** (Layer 0, optional)

## License

Copyright (c) Jan Ohlmann. All rights reserved.
Licensed to ANTARES SIMULACAO LTDA (CNPJ 69.363.366/0001-21) under ASE-JAN-LTDA-2026-10-16-1.0.

This repository is released under the Antares Simulation Engine and ADG License Agreement
**ASE-ADG-LIC-2026-10-16-1.0** ([LICENSE.md](LICENSE.md)) and the ADG Project Participation
Agreement **ASE-ADG-PJV-2026-10-16-1.0** ([PJV.md](PJV.md)). Contribution workflow: [CONTRIBUTING.md](CONTRIBUTING.md).

Translations: [DE LIC](license/de/LIC_ASE_ADG_DE.md) · [DE PJV](license/de/PJV_ASE_ADG_DE.md) ·
[PT LIC](license/pt/LIC_ASE_ADG_PT.md) · [PT PJV](license/pt/PJV_ASE_ADG_PT.md)

SPDX-License-Identifier: LicenseRef-ASE-LIC

Third-party components keep their own licenses; see [NOTICE](NOTICE).
