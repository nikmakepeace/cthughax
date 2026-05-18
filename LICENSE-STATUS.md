# License Status

This document summarizes the licensing notices currently present in this
repository. It is not legal advice.

## Current Repository Notices

- `README` states that CthughaNix is a relicensing, renaming, and
  modernization of the Cthugha-L distribution, and refers readers to
  `COPYING` for license details.
- `COPYING` states that it is "based on the LGPL License" and refers to
  `lgpl.txt`, but its operative text appears to be a copyright attribution
  summary and a warranty disclaimer rather than a complete copying permission
  grant.
- `LICENSE` contains the text of the GNU Lesser General Public License,
  version 2.1.
- `lgpl.txt` contains, or appears to contain, text from the GNU Lesser General
  Public License version 3.

The repository does not currently contain a clear license notice stating that
the whole work is licensed under a specific version of the LGPL, nor does it
clearly identify which files, if any, are intended to be LGPL-licensed.

## Cthugha and Cthugha-L Notices

The Cthugha and Cthugha-L materials included in this repository contain
historical shareware and source-use terms.

Known notices include:

- The original Cthugha source notice allowed copying and use in other programs,
  but stated that the source code may not be used in a commercial program
  without prior permission, and requested credit to Cthugha and Torps
  Productions.
- The Cthugha-L `COPYING` text similarly allowed copying and use in other
  programs, but stated that the source code may not be used in a commercial
  program without prior permission, and requested credit to Cthugha, Torps
  Productions, and Harald Deischinger.
- The historical notices also requested that modified versions not be
  distributed independently, and instead asked that changes be sent back to the
  original maintainer for possible official release.
- `cthugha-L.lsm` identifies the Cthugha-L copying policy as "shareware".
- `doc/cthugha.1` contains shareware terms describing non-commercial use and
  commercial registration.

The historical notices appear to distinguish between source-use terms and
binary/shareware distribution terms. The source notices address copying, use in
other programs, attribution, modification, and non-commercial restrictions. The
shareware notices address non-commercial binary use, commercial registration,
redistribution with documentation intact, and distribution by shareware
houses/stores.

These notices are not the same as the LGPL text included in this repository.

## Copyright Attributions

`COPYING` states the following:

- Some parts of the source are public domain.
- Some parts of the source bear explicit copyright notices and are subject to
  the conditions listed by those authors.
- The remainder of the source, where not otherwise public domain or covered by
  an explicit notice, is copyright 1995-1997 by Harald Deischinger.
- New modifications are copyright 2007 by Brandon Barker or other authors as
  described in the source.

This attribution summary does not by itself establish that prior Cthugha or
Cthugha-L material was assigned or validly relicensed.

The source tree also contains files with separate notices, including third-party
permissive notices such as `src/vroot.h`.

## Current Status

Based on the files present in this repository and in the original Cthugha 5.3 
source:

- The historical Cthugha and Cthugha-L source and binary distribution notices
  remain present.
- The repository includes LGPL license texts.
- The repository describes CthughaNix as a relicensing of Cthugha-L-derived
  material, but does not include evidence that all prior copyright holders
  granted permission for such relicensing.
- The licensing status of the combined repository is therefore unclear.
- The combined repository should not be treated as clearly LGPL-licensed as a
  whole based only on the files currently present.

Until this is clarified, downstream users should review the historical
Cthugha/Cthugha-L terms, the included LGPL texts, and the file-specific notices
before redistributing, modifying, or using the software commercially.