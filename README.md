# COMS3008A Assignment 1 – Setup & Run Guide

## Folder structure
```
assignment1/
├── .devcontainer/
│   └── devcontainer.json   ← tells Codespaces what to install
├── output/                 ← generated images go here (created by run.sh)
├── pics/
│   └── witslogo_h.png      ← needed for LaTeX report (copy from provided files)
├── fractal.cpp             ← ALL your parallel implementations
├── Makefile
├── run.sh
├── main.tex                ← root LaTeX file
├── assignment_report.tex   ← report content
├── references.bib          ← bibliography
└── README.md               ← this file
```

---

## STEP 1 – Open in GitHub Codespaces

1. Go to https://github.com and create a **new repository** (name it e.g. `coms3008a-assign1`)
2. Upload ALL files from this folder into it
3. Click the green **Code** button → **Codespaces** tab → **Create codespace on main**
4. Wait ~2 minutes while Codespaces sets up the environment automatically
   (it installs g++, OpenMP, LaTeX, Make via `.devcontainer/devcontainer.json`)

---

## STEP 2 – Compile and run the fractal code

Open the terminal inside Codespaces and run:

```bash
make clean
make
bash run.sh
```

You will see output like:
```
=== Julia Set Fractal Benchmark (DIM=768) ===
Serial time: 1.234 s

Threads  1D_Row_t   1D_Col_t  ...  1D_Row_S  ...
1        1.23       1.25      ...  1.00      ...
2        0.63       0.68      ...  1.96      ...
...
```

**Copy this entire output** — you need the speedup numbers (the S columns) for your report.

---

## STEP 3 – Put the Wits logo in place

The LaTeX report needs the Wits logo. Copy `witslogo_h.png` into the `pics/` folder:

```bash
mkdir -p pics
cp /path/to/witslogo_h.png pics/
```

(You uploaded `witslogo_h.png` — it's available in your repo already if you uploaded it.)

---

## STEP 4 – Fill in your results in the report

Open `assignment_report.tex` and:
1. Replace `Your Full Name` and `XXXXXXX` with your actual name and student number
2. Replace all `FILL` entries in Table~1 with your actual speedup numbers from `terminal.out`
3. Fill in your machine specs (number of cores, RAM, CPU model) in the table caption
   - In Codespaces you can check: `nproc` (cores), `lscpu` (CPU details)

---

## STEP 5 – Compile the LaTeX report

```bash
pdflatex main.tex
bibtex main
pdflatex main.tex
pdflatex main.tex
```

This produces `main.pdf` — your report.

---

## STEP 6 – Package your submission

```bash
zip -r submission.zip fractal.cpp Makefile run.sh main.tex assignment_report.tex references.bib pics/ output/
```

Submit `submission.zip` and `main.pdf` on Ulwazi.

---

## Troubleshooting

| Problem | Fix |
|---------|-----|
| `make: g++: not found` | Run `sudo apt-get install -y g++` |
| `fatal error: omp.h not found` | Run `sudo apt-get install -y libomp-dev` |
| `pdflatex: command not found` | Run `sudo apt-get install -y texlive-full` |
| Images not saving | Make sure `output/` folder exists: `mkdir -p output` |
| Codespace timeout | Your free Codespaces hours reset monthly; use the terminal quickly |