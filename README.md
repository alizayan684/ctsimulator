# CT Simulator - Educational Platform for Computed Tomography

A comprehensive educational tool for learning and experimenting with X-ray Computed Tomography (CT) simulation, acquisition, and image reconstruction algorithms.

## 📋 Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Architecture](#architecture)
- [Reconstruction Algorithms](#reconstruction-algorithms)
- [Visualization Tools](#visualization-tools)
- [Educational Tutorials](#educational-tutorials)
- [Python Implementation Guide](#python-implementation-guide)

---

## 🎯 Overview

CT Simulator is an educational platform designed to teach the principles of Computed Tomography through interactive simulation. The software allows users to:

1. **Load or generate phantom images** (standard Shepp-Logan or custom phantoms)
2. **Simulate X-ray CT acquisition** with configurable parameters
3. **Generate system matrices** using Siddon's ray-tracing algorithm
4. **Create sinograms** with optional noise injection
5. **Reconstruct images** using multiple iterative and analytical algorithms
6. **Visualize and analyze** results with advanced tools

The platform bridges theoretical CT physics with practical implementation, making it ideal for students, researchers, and educators in medical imaging.

---

## ✨ Features

### Core Simulation Features

#### 1. Phantom Management
- **Built-in Shepp-Logan Phantom**: Generate the standard CT test phantom (256×256)
- **Custom Phantom Loading**: Import phantom data from tab-separated text files
- **Phantom Visualization**: Real-time display of phantom images with normalization

#### 2. CT Acquisition Simulation
- **Configurable Geometry**: 
  - Source and detector array positioning
  - Rotational gantry movement (0° to 360°)
  - Adjustable number of projections
- **System Matrix Generation**: 
  - Siddon's 2D ray-tracing algorithm
  - Accurate chord length calculation through pixels
  - Sparse matrix representation
- **Sinogram Generation**:
  - Forward projection simulation
  - Gaussian noise injection (configurable voltage/current)
  - Box-Muller transform for noise generation

#### 3. Reconstruction Algorithms
Four complete reconstruction algorithms with unified interface:

| Algorithm | Type | Key Features |
|-----------|------|--------------|
| **ART** | Iterative | Algebraic Reconstruction Technique, row-by-row updates, relaxation parameter |
| **SIRT** | Iterative | Simultaneous Iterative Reconstruction, full-matrix updates per iteration |
| **FBP** | Analytical | Filtered Back Projection with 5 filter options, FFT-based filtering |
| **MLEM** | Statistical | Maximum Likelihood Expectation Maximization, Poisson noise model |

#### 4. Parameter Control
- **Iterations**: Configurable sweep count (1-1000+)
- **Relaxation Parameter**: Damping factor for iterative algorithms (0.0-1.0)
- **Update Frequency**: Control visualization update interval (showEvery parameter)
- **Filter Selection**: Choose from 5 reconstruction filters for FBP
- **Presets**: Quick Preview, Standard, and High Quality configurations

#### 5. Session Management
- **Save/Load Sessions**: Export all parameters to JSON format
- **Undo/Redo**: Full parameter change history (up to 50 states)
- **Image Export**: Save reconstructions and sinograms as PNG, TIFF, JPEG, BMP

---

## 🏗️ Architecture

### Backend (C++17/Qt6)

```
source/                     # Core CT simulation engine
├── CTController.h/.cpp     # Main controller, QML bridge, async orchestration
├── CTImageProvider.h/.cpp  # QImage provider for QML image elements
├── drawingarea.h/.cpp      # System matrix & sinogram generation
├── matrix.h/.cpp           # Matrix data structure with QImage rendering
└── utility.h/.cpp          // Siddon's algorithm, phantom file loading

src/reconstruction/         # Reconstruction algorithms
├── ReconstructionAlgorithm # Abstract base class (QThread)
├── ART.h/.cpp              # Algebraic Reconstruction Technique
├── SIRT.h/.cpp             # Simultaneous Iterative Reconstruction
├── FBP.h/.cpp              # Filtered Back Projection + FFT
└── MLEM.h/.cpp             # Maximum Likelihood Expectation Maximization

src/visualization/          # Advanced visualization components
├── ColormapManager         # Viridis, Plasma, Inferno, Magma colormaps
├── ImageViewer             # QQuickPaintedItem for high-quality rendering
├── MetricsCalculator       # RMSE, PSNR, SSIM computation
├── ROITools                # Histograms, line profiles
└── RayPathVisualizer       # Interactive ray geometry display
```

### Frontend (QML/Qt Quick)

```
Main.qml                    # Application window, navigation tabs
MainView.qml                # Main simulator interface
qml/tutorials/              # Educational content
├── TutorialManager.qml     # Tutorial orchestrator
├── IntroductionTutorial.qml
├── ProjectionsTutorial.qml
├── AlgorithmsTutorial.qml
├── NoiseTutorial.qml
└── OptimizationTutorial.qml
```

### Threading Model

- **Main Thread**: UI rendering, user input handling
- **Worker Thread 1**: Sinogram generation (QtConcurrent)
- **Worker Thread 2**: Reconstruction algorithm (QThread subclass)
- **Signal/Slot Communication**: QueuedConnection for thread-safe updates

---

## 🔬 Reconstruction Algorithms

### 1. ART (Algebraic Reconstruction Technique)

**Mathematical Foundation:**
```
For each projection i:
  x_new = x_old + λ * (b_i - A_i·x_old) / ||A_i||² * A_i^T
```

**Implementation Details:**
- Row-by-row iterative updates
- Relaxation parameter (λ) controls convergence speed
- Residual monitoring: L1 norm vs. phantom
- Early termination support

**Best For:** Limited projection data, sparse systems

---

### 2. SIRT (Simultaneous Iterative Reconstruction Technique)

**Mathematical Foundation:**
```
1. Compute residual: r = b - Ax
2. Backproject: Δx = A^T · r
3. Update: x_new = x_old + λ · Δx
```

**Implementation Details:**
- Full-matrix update per iteration
- More stable than ART but slower convergence
- Better noise handling

**Best For:** Noisy data, uniform convergence requirements

---

### 3. FBP (Filtered Back Projection)

**Mathematical Foundation:**
```
1. Apply ramp filter in frequency domain: H(ω) = |ω|
2. Multiply by window function W(ω)
3. Inverse FFT to get filtered projections
4. Backproject: f(x,y) = ∫ p_θ(t) dθ
```

**Available Filters:**
| Filter | Formula | Characteristics |
|--------|---------|-----------------|
| Ram-Lak | |ω| | Sharp, noise-amplifying |
| Shepp-Logan | |ω| · sinc(πω/2ω_max) | Smoothed, reduced noise |
| Cosine | |ω| · cos(πω/2ω_max) | Moderate smoothing |
| Hamming | |ω| · (0.54 + 0.46·cos(πω/ω_max)) | Strong noise suppression |
| Hann | |ω| · (0.5 + 0.5·cos(πω/ω_max)) | Maximum smoothing |

**Implementation Details:**
- Iterative Cooley-Tukey FFT (power-of-two length)
- Linear interpolation during backprojection
- Single-pass reconstruction (non-iterative)

**Best For:** Complete data, fast reconstruction, clinical applications

---

### 4. MLEM (Maximum Likelihood Expectation Maximization)

**Mathematical Foundation:**
```
x_j^{new} = x_j^{old} · [A^T(b / Ax^{old})]_j / [A^T·1]_j
```

**Implementation Details:**
- Poisson likelihood model for photon counting
- Sensitivity image precomputation (A^T·1)
- Positivity enforcement (no negative values)
- Relative change convergence criterion (1e-6 threshold)
- Relaxation as dampening factor

**Best For:** Low-count PET/SPECT, Poisson-distributed noise

---

## 🎨 Visualization Tools

### 1. ImageViewer
- **High-Quality Rendering**: Anti-aliased, smooth scaling
- **Zoom/Pan**: Interactive exploration of fine details
- **Aspect Ratio Preservation**: Correct geometric representation

### 2. ColormapManager
- **Supported Colormaps**: Grayscale, Viridis, Plasma, Inferno, Magma
- **Perceptual Uniformity**: Viridis family for accurate feature detection
- **Real-Time Application**: Instant colormap switching

### 3. MetricsCalculator
- **RMSE (Root Mean Square Error)**: Overall reconstruction accuracy
- **PSNR (Peak Signal-to-Noise Ratio)**: Quality metric in dB
- **SSIM (Structural Similarity Index)**: Perceptual similarity (planned)

### 4. ROITools
- **Histogram Analysis**: Pixel intensity distribution in selected regions
- **Line Profiles**: Intensity variation along user-defined paths
- **Statistical Metrics**: Mean, std dev, min/max in ROI

### 5. RayPathVisualizer
- **Interactive Geometry**: Adjust angle and offset in real-time
- **Source/Detector Display**: Visual representation of CT hardware
- **Grid Overlay**: Reference for spatial understanding

---

## 📚 Educational Tutorials

### Tutorial 1: Introduction to CT
- Basic principles of computed tomography
- Historical context and applications
- Animated explanation of projection formation
- Interactive phantom exploration

### Tutorial 2: How Projections Work
- Step-by-step sinogram generation
- Relationship between object and sinogram
- Effect of object position on sinogram patterns
- Hands-on projection parameter adjustment

### Tutorial 3: Reconstruction Algorithms Compared
- Side-by-side algorithm comparison
- Parameter sensitivity demonstration
- Convergence behavior visualization
- Strengths and weaknesses of each method

### Tutorial 4: Noise and Artifacts
- Types of CT artifacts (beam hardening, metal, motion)
- Noise modeling and injection
- Effect of limited angles and sparse sampling
- Strategies for artifact reduction

### Tutorial 5: Parameter Optimization
- Guided workflow for optimal settings
- Trade-offs: speed vs. quality
- Algorithm-specific tuning tips
- Preset configuration explanations

---

