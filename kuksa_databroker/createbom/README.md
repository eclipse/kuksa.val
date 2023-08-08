# BOM Generator

Generates a BOM

## Troubleshooting

If you run it and you get errors like:

```
Could not find license file for 0BSD in adler
Error: BOM creation failed, unresolved licenses detected
```

The a possible reason is that the `Cargo.lock` in the repository has been updated and some components use licenses
not covered here. This can be solved by:

* Find the corresponding license test. Check for instance [SPDX](https://spdx.org/licenses/)
* Verify that license is feasible for our use.
* Download or create a text file with the license text, do `gzip` and put it in `licensestore` folder.
* Add the identifier (in the example above `0BSD`) to `maplicensefile.py`
