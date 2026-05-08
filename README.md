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

## 🐍 Python Implementation Guide

This section provides a detailed breakdown for reimplementing this project in Python. The implementation is divided into logical modules with specific subtasks.

### Phase 1: Core Data Structures & Utilities

#### Module 1.1: Matrix Operations
**File:** `core/matrix.py`

```python
# Subtasks:
# 1.1.1 Create Matrix class with NumPy array backend
# 1.1.2 Implement row-major data storage
# 1.1.3 Add bounds-checked element access (.at method)
# 1.1.4 Implement QImage-like rendering (convert to PIL/grayscale)
# 1.1.5 Add serialization/deserialization methods
# 1.1.6 Implement min/max normalization for display
```

**Key Classes:**
```python
class Matrix:
    def __init__(self, data: np.ndarray)
    def at(self, row: int, col: int) -> float
    def to_image(self) -> Image.Image
    @classmethod
    def load_from_file(cls, filename: str) -> 'Matrix'
```

---

#### Module 1.2: Siddon's Ray-Tracing Algorithm
**File:** `core/ray_tracing.py`

```python
# Subtasks:
# 1.2.1 Implement Point2D named tuple/class
# 1.2.2 Handle vertical ray case (cos ≈ 0)
# 1.2.3 Handle horizontal ray case (|cos| ≈ 1)
# 1.2.4 Implement diagonal ray crossing point calculation
# 1.2.5 Merge horizontal and vertical crossing points
# 1.2.6 Filter out-of-bounds crossing points
# 1.2.7 Calculate chord lengths through pixels
# 1.2.8 Return flat ray vector (row-major order)
# 1.2.9 Add unit tests for each ray orientation
```

**Key Functions:**
```python
def siddon_ray_trace_2d(
    source: Tuple[float, float],
    detector: Tuple[float, float],
    rows: int,
    cols: int
) -> np.ndarray:
    """Calculate chord lengths through each pixel."""
```

---

#### Module 1.3: Phantom Generation
**File:** `core/phantom.py`

```python
# Subtasks:
# 1.3.1 Implement Shepp-Logan phantom generator
# 1.3.2 Define ellipse parameters (intensity, center, axes, rotation)
# 1.3.3 Vectorized ellipse evaluation using NumPy broadcasting
# 1.3.4 Support custom phantom loading from TSV files
# 1.3.5 Add phantom visualization helper functions
```

**Key Functions:**
```python
def shepp_logan(size: int = 256) -> np.ndarray
def load_phantom(filename: str) -> np.ndarray
def create_custom_phantom(ellipses: List[EllipseParams]) -> np.ndarray
```

---

### Phase 2: CT Acquisition System

#### Module 2.1: Source and Detector Geometry
**File:** `acquisition/geometry.py`

```python
# Subtasks:
# 2.1.1 Create CTGeometry class
# 2.1.2 Implement source array positioning (complex numbers or vectors)
# 2.1.3 Implement detector array positioning
# 2.1.4 Calculate source-detector distance (R parameter)
# 2.1.5 Add element spacing calculation
# 2.1.6 Implement gantry rotation (rotation matrix multiplication)
# 2.1.7 Store projected source/detector positions per angle
```

**Key Classes:**
```python
@dataclass
class CTGeometry:
    num_source_elements: int
    num_detector_elements: int
    num_projections: int
    source_positions: np.ndarray  # shape: (projections, elements, 2)
    detector_positions: np.ndarray
    
    def rotate_gantry(self, start_angle: float, end_angle: float, increment: float)
```

---

#### Module 2.2: System Matrix Construction
**File:** `acquisition/system_matrix.py`

```python
# Subtasks:
# 2.2.1 Create SystemMatrixBuilder class
# 2.2.2 Initialize sparse matrix structure (scipy.sparse)
# 2.2.3 Iterate over all source-detector pairs
# 2.2.4 Call Siddon's algorithm for each ray
# 2.2.5 Store ray vectors as matrix rows
# 2.2.6 Implement forward projection (A · x)
# 2.2.7 Implement backprojection (A^T · y)
# 2.2.8 Add error handling for failed ray traces
# 2.2.9 Optimize with parallel processing (multiprocessing/joblib)
```

**Key Classes:**
```python
class SystemMatrixBuilder:
    def __init__(self, geometry: CTGeometry, phantom_shape: Tuple[int, int])
    def build(self) -> scipy.sparse.csr_matrix
    def forward_project(self, image: np.ndarray) -> np.ndarray
    def backproject(self, sinogram: np.ndarray) -> np.ndarray
```

---

#### Module 2.3: Sinogram Generation
**File:** `acquisition/sinogram.py`

```python
# Subtasks:
# 2.3.1 Create SinogramGenerator class
# 2.3.2 Implement noiseless forward projection
# 2.3.3 Add Gaussian noise injection (Box-Muller or NumPy random)
# 2.3.4 Calculate noise standard deviation from kVp/mAs parameters
# 2.3.5 Support Poisson noise model for photon counting
# 2.3.6 Export sinogram as image (PNG/TIFF)
# 2.3.7 Visualize sinogram with matplotlib
```

**Key Classes:**
```python
class SinogramGenerator:
    def __init__(self, system_matrix: scipy.sparse.csr_matrix)
    
    def generate(
        self,
        phantom: np.ndarray,
        voltage: float = 120.0,
        current: float = 100.0,
        noise_std: Optional[float] = None
    ) -> np.ndarray
```

---

### Phase 3: Reconstruction Algorithms

#### Module 3.1: Abstract Base Class
**File:** `reconstruction/base.py`

```python
# Subtasks:
# 3.1.1 Create abstract ReconstructionAlgorithm base class
# 3.1.2 Define common properties (iterations, relaxation, show_every)
# 3.1.3 Implement abstract reconstruct() method
# 3.1.4 Add progress callback mechanism (generator or callable)
# 3.1.5 Implement early stopping based on convergence
# 3.1.6 Add thread-safety with threading.Event for cancellation
```

**Key Classes:**
```python
from abc import ABC, abstractmethod

class ReconstructionAlgorithm(ABC):
    def __init__(
        self,
        iterations: int = 50,
        relaxation: float = 0.1,
        show_every: int = 1,
        callback: Optional[Callable] = None
    ):
        self.iterations = iterations
        self.relaxation = relaxation
        self.show_every = show_every
        self.callback = callback
        self._stop_event = threading.Event()
    
    @abstractmethod
    def reconstruct(
        self,
        system_matrix: scipy.sparse.csr_matrix,
        sinogram: np.ndarray,
        initial_guess: Optional[np.ndarray] = None
    ) -> np.ndarray:
        pass
    
    def stop(self):
        self._stop_event.set()
```

---

#### Module 3.2: ART Implementation
**File:** `reconstruction/art.py`

```python
# Subtasks:
# 3.2.1 Extend ReconstructionAlgorithm base class
# 3.2.2 Implement row-by-row update loop
# 3.2.3 Calculate numerator (A_i · x)
# 3.2.4 Calculate denominator (||A_i||²)
# 3.2.5 Apply relaxation-weighted correction
# 3.2.6 Emit intermediate results every show_every iterations
# 3.2.7 Calculate L1 residual vs. reference (if provided)
# 3.2.8 Optimize with sparse matrix operations
```

**Implementation:**
```python
class ART(ReconstructionAlgorithm):
    def reconstruct(
        self,
        system_matrix: scipy.sparse.csr_matrix,
        sinogram: np.ndarray,
        initial_guess: Optional[np.ndarray] = None
    ) -> np.ndarray:
        A = system_matrix.tolil()  # Convert to list format for row access
        b = sinogram.flatten()
        n_pixels = A.shape[1]
        
        x = initial_guess if initial_guess is not None else np.zeros(n_pixels)
        
        for sweep in range(self.iterations):
            if self._stop_event.is_set():
                break
                
            for i in range(A.shape[0]):
                row = A.getrow(i).toarray().flatten()
                
                numerator = np.dot(row, x)
                denominator = np.dot(row, row)
                
                if denominator > 1e-12:
                    update = (b[i] - numerator) / denominator
                    x += self.relaxation * row * update
            
            if (sweep + 1) % self.show_every == 0:
                if self.callback:
                    self.callback(sweep + 1, x.copy())
        
        return x.reshape(phantom_shape)
```

---

#### Module 3.3: SIRT Implementation
**File:** `reconstruction/sirt.py`

```python
# Subtasks:
# 3.3.1 Extend ReconstructionAlgorithm base class
# 3.3.2 Compute full residual vector (r = b - Ax)
# 3.3.3 Backproject residual (A^T · r)
# 3.3.4 Apply simultaneous update to all pixels
# 3.3.5 Implement relaxation damping
# 3.3.6 Add convergence monitoring
# 3.3.7 Compare performance with ART
```

**Implementation:**
```python
class SIRT(ReconstructionAlgorithm):
    def reconstruct(
        self,
        system_matrix: scipy.sparse.csr_matrix,
        sinogram: np.ndarray,
        initial_guess: Optional[np.ndarray] = None
    ) -> np.ndarray:
        A = system_matrix
        b = sinogram.flatten()
        
        x = initial_guess if initial_guess is not None else np.zeros(A.shape[1])
        
        # Precompute row and column sums for normalization
        row_sum = np.array(A.abs().sum(axis=1)).flatten()
        col_sum = np.array(A.abs().sum(axis=0)).flatten()
        
        for sweep in range(self.iterations):
            if self._stop_event.is_set():
                break
            
            # Compute residual
            ax = A.dot(x)
            residual = b - ax
            
            # Backproject
            backproj = A.T.dot(residual)
            
            # Normalize and update
            update = backproj / (col_sum + 1e-12)
            x += self.relaxation * update
            
            if (sweep + 1) % self.show_every == 0:
                if self.callback:
                    self.callback(sweep + 1, x.copy())
        
        return x.reshape(phantom_shape)
```

---

#### Module 3.4: FBP Implementation
**File:** `reconstruction/fbp.py`

```python
# Subtasks:
# 3.4.1 Implement FFT (or use scipy.fft)
# 3.4.2 Create ramp filter builder function
# 3.4.3 Implement all 5 filter types (Ram-Lak, Shepp-Logan, Cosine, Hamming, Hann)
# 3.4.4 Apply filter in frequency domain
# 3.4.5 Inverse FFT to get filtered projections
# 3.4.6 Implement backprojection with linear interpolation
# 3.4.7 Handle detector geometry mapping
# 3.4.8 Normalize by number of projections
# 3.4.9 Add GPU acceleration option (CuPy)
```

**Implementation:**
```python
from scipy import fft

class FBP(ReconstructionAlgorithm):
    FILTER_TYPES = ['ram-lak', 'shepp-logan', 'cosine', 'hamming', 'hann']
    
    def __init__(self, filter_type: str = 'ram-lak', **kwargs):
        super().__init__(**kwargs)
        self.filter_type = filter_type
    
    def _build_filter(self, n: int) -> np.ndarray:
        """Build frequency-domain filter."""
        freq = np.fft.rfftfreq(n)
        
        if self.filter_type == 'ram-lak':
            h = np.abs(freq)
        elif self.filter_type == 'shepp-logan':
            h = np.abs(freq) * np.sinc(freq / 2)
        elif self.filter_type == 'cosine':
            h = np.abs(freq) * np.cos(np.pi * freq / 2)
        elif self.filter_type == 'hamming':
            h = np.abs(freq) * (0.54 + 0.46 * np.cos(np.pi * freq))
        elif self.filter_type == 'hann':
            h = np.abs(freq) * (0.5 + 0.5 * np.cos(np.pi * freq))
        
        h[0] = 0  # Suppress DC
        return h
    
    def reconstruct(
        self,
        system_matrix: scipy.sparse.csr_matrix,
        sinogram: np.ndarray,
        initial_guess: Optional[np.ndarray] = None
    ) -> np.ndarray:
        # Note: FBP doesn't use system_matrix directly
        # It works with parallel-beam geometry assumptions
        
        n_projections, n_detectors = sinogram.shape
        
        # Pad to power of 2 for FFT efficiency
        n_fft = 2 ** int(np.ceil(np.log2(n_detectors)))
        
        # Build filter
        filt = self._build_filter(n_fft)
        
        # Filter each projection
        filtered = np.zeros_like(sinogram)
        for p in range(n_projections):
            proj = sinogram[p, :]
            proj_padded = np.pad(proj, (0, n_fft - n_detectors))
            
            # FFT → multiply → inverse FFT
            proj_fft = np.fft.rfft(proj_padded)
            filtered_fft = proj_fft * filt
            filtered_proj = np.fft.irfft(filtered_fft)[:n_detectors]
            
            filtered[p, :] = filtered_proj
        
        # Backprojection
        image = self._backproject(filtered, n_projections)
        
        return image
    
    def _backproject(self, filtered_sinogram: np.ndarray, n_angles: int) -> np.ndarray:
        """Perform filtered backprojection."""
        # Assuming square image
        img_size = filtered_sinogram.shape[1]
        image = np.zeros((img_size, img_size))
        
        cx, cy = img_size / 2, img_size / 2
        
        for p, angle in enumerate(np.linspace(0, np.pi, n_angles, endpoint=False)):
            cos_a, sin_a = np.cos(angle), np.sin(angle)
            
            for y in range(img_size):
                for x in range(img_size):
                    # Calculate detector coordinate
                    t = (x - cx) * cos_a + (y - cy) * sin_a
                    
                    # Map to detector index with interpolation
                    det_idx = t + n_angles / 2
                    i0 = int(np.floor(det_idx))
                    i1 = i0 + 1
                    w1 = det_idx - i0
                    
                    if 0 <= i0 < n_angles and 0 <= i1 < n_angles:
                        image[y, x] += (1 - w1) * filtered_sinogram[p, i0] + \
                                       w1 * filtered_sinogram[p, i1]
        
        return image / n_angles
```

---

#### Module 3.5: MLEM Implementation
**File:** `reconstruction/mlem.py`

```python
# Subtasks:
# 3.5.1 Extend ReconstructionAlgorithm base class
# 3.5.2 Precompute sensitivity image (A^T · 1)
# 3.5.3 Implement forward projection (Ax)
# 3.5.4 Calculate ratio (b / Ax) with epsilon protection
# 3.5.5 Backproject ratio (A^T · ratio)
# 3.5.6 Apply MLEM update formula
# 3.5.7 Enforce positivity constraint
# 3.5.8 Implement relative change convergence criterion
# 3.5.9 Add relaxation as dampening factor
```

**Implementation:**
```python
class MLEM(ReconstructionAlgorithm):
    def reconstruct(
        self,
        system_matrix: scipy.sparse.csr_matrix,
        sinogram: np.ndarray,
        initial_guess: Optional[np.ndarray] = None
    ) -> np.ndarray:
        A = system_matrix
        b = sinogram.flatten()
        n_pixels = A.shape[1]
        
        # Initial uniform estimate (must be positive)
        x = initial_guess if initial_guess is not None else np.ones(n_pixels)
        
        # Precompute sensitivity image: s = A^T · 1
        sensitivity = np.array(A.T.dot(np.ones(A.shape[0]))).flatten()
        sensitivity[sensitivity < 1e-12] = 1.0  # Avoid division by zero
        
        prev_residual = np.inf
        
        for sweep in range(self.iterations):
            if self._stop_event.is_set():
                break
            
            x_prev = x.copy()
            
            # Forward project
            ax = A.dot(x_prev) + 1e-12
            
            # Calculate ratio
            ratio = b / ax
            
            # Backproject ratio
            backproj = np.array(A.T.dot(ratio)).flatten()
            
            # MLEM update with relaxation
            update = x_prev * backproj / sensitivity
            x = x_prev + self.relaxation * (update - x_prev)
            
            # Enforce positivity
            x[x < 1e-12] = 1e-12
            
            if (sweep + 1) % self.show_every == 0:
                if self.callback:
                    self.callback(sweep + 1, x.copy())
            
            # Check convergence
            residual = np.sum(np.abs(x - x_prev))
            if prev_residual != np.inf:
                rel_change = abs(prev_residual - residual) / (prev_residual + 1e-12)
                if rel_change < 1e-6:
                    break
            prev_residual = residual
        
        return x.reshape(phantom_shape)
```

---

### Phase 4: Visualization & Analysis

#### Module 4.1: Image Viewer
**File:** `visualization/viewer.py`

```python
# Subtasks:
# 4.1.1 Create ImageViewer class using matplotlib or PyQtGraph
# 4.1.2 Implement zoom functionality (mouse wheel)
# 4.1.3 Implement pan functionality (mouse drag)
# 4.1.4 Add colormap application
# 4.1.5 Support side-by-side comparison mode
# 4.1.6 Add difference image overlay
# 4.1.7 Implement smooth interpolation for zoom
```

**Key Classes:**
```python
class ImageViewer:
    def __init__(self, cmap: str = 'viridis')
    def set_image(self, image: np.ndarray)
    def apply_colormap(self, cmap_name: str)
    def enable_comparison_mode(self, image1: np.ndarray, image2: np.ndarray)
    def show_difference(self)
```

---

#### Module 4.2: Colormap Manager
**File:** `visualization/colormap.py`

```python
# Subtasks:
# 4.2.1 Implement Viridis colormap lookup table
# 4.2.2 Implement Plasma, Inferno, Magma colormaps
# 4.2.3 Add grayscale conversion
# 4.2.4 Normalize data to [0, 1] range
# 4.2.5 Handle NaN and infinite values
# 4.2.6 Support custom colormap registration
```

---

#### Module 4.3: Metrics Calculator
**File:** `visualization/metrics.py`

```python
# Subtasks:
# 4.3.1 Implement RMSE calculation
# 4.3.2 Implement PSNR calculation
# 4.3.3 Implement SSIM (using scikit-image or custom)
# 4.3.4 Add multi-scale SSIM option
# 4.3.5 Create metrics dashboard display
# 4.3.6 Export metrics to CSV/JSON
```

**Implementation:**
```python
def calculate_rmse(reference: np.ndarray, target: np.ndarray) -> float:
    return np.sqrt(np.mean((reference - target) ** 2))

def calculate_psnr(reference: np.ndarray, target: np.ndarray, max_val: float = 1.0) -> float:
    rmse = calculate_rmse(reference, target)
    if rmse == 0:
        return float('inf')
    return 20 * np.log10(max_val / rmse)

def calculate_ssim(reference: np.ndarray, target: np.ndarray, 
                   window_size: int = 11) -> float:
    # Use scikit-image or implement from scratch
    from skimage.metrics import structural_similarity
    return structural_similarity(reference, target, data_range=target.max() - target.min())
```

---

#### Module 4.4: ROI Tools
**File:** `visualization/roi_tools.py`

```python
# Subtasks:
# 4.4.1 Implement rectangular ROI selection
# 4.4.2 Calculate histogram for ROI (NumPy histogram)
# 4.4.3 Implement Bresenham's line algorithm for line profiles
# 4.4.4 Extract intensity values along line
# 4.4.5 Plot histogram and line profile
# 4.4.6 Calculate statistics (mean, std, min, max)
# 4.4.7 Support multiple ROI definitions
```

---

#### Module 4.5: Ray Path Visualizer
**File:** `visualization/ray_visualizer.py`

```python
# Subtasks:
# 4.5.1 Create interactive matplotlib widget
# 4.5.2 Draw source and detector positions
# 4.5.3 Animate ray path for selected angle
# 4.5.4 Show grid overlay
# 4.5.5 Add angle slider control
# 4.5.6 Add offset slider control
# 4.5.7 Highlight current ray in system matrix
```

---

### Phase 5: User Interface & Integration

#### Module 5.1: Controller (MVC Pattern)
**File:** `ui/controller.py`

```python
# Subtasks:
# 5.1.1 Create CTController class (model-view-controller)
# 5.1.2 Implement phantom loading/generation methods
# 5.1.3 Implement sinogram generation workflow
# 5.1.4 Implement reconstruction orchestration
# 5.1.5 Add async execution with threading/asyncio
# 5.1.6 Implement progress reporting (callbacks/events)
# 5.1.7 Add error handling and validation
# 5.1.8 Implement undo/redo stack for parameter changes
# 5.1.9 Add session save/load (JSON serialization)
```

---

#### Module 5.2: GUI Framework
**File:** `ui/gui.py`

**Option A: PyQt6/PySide6**
```python
# Subtasks:
# 5.2.A.1 Create main window with QMainWindow
# 5.2.A.2 Design tabbed interface (QTabWidget)
# 5.2.A.3 Build control panels with Qt Widgets
# 5.2.A.4 Embed matplotlib figures (FigureCanvasQTAgg)
# 5.2.A.5 Implement signal/slot connections
# 5.2.A.6 Add threaded worker with QThread
# 5.2.A.7 Create progress dialogs
```

**Option B: Streamlit (Web-based)**
```python
# Subtasks:
# 5.2.B.1 Create streamlit app structure
# 5.2.B.2 Build sidebar controls
# 5.2.B.3 Use st.pyplot for visualization
# 5.2.B.4 Implement session state management
# 5.2.B.5 Add file upload/download widgets
```

**Option C: Tkinter (Simple Desktop)**
```python
# Subtasks:
# 5.2.C.1 Create main window with ttk.Notebook
# 5.2.C.2 Design control frames
# 5.2.C.3 Embed matplotlib with TkAgg backend
# 5.2.C.4 Implement threading for long operations
```

---

#### Module 5.3: Tutorial System
**File:** `ui/tutorials.py`

```python
# Subtasks:
# 5.3.1 Create Tutorial base class
# 5.3.2 Implement step-by-step wizard interface
# 5.3.3 Add interactive demonstrations
# 5.3.4 Create quiz/knowledge check questions
# 5.3.5 Implement progress tracking
# 5.3.6 Add multimedia content (images, animations)
# 5.3.7 Build 5 complete tutorials:
#   - Introduction to CT
#   - How Projections Work
#   - Reconstruction Algorithms Compared
#   - Noise and Artifacts
#   - Parameter Optimization
```

---

### Phase 6: Testing & Optimization

#### Module 6.1: Unit Tests
**File:** `tests/test_*.py`

```python
# Subtasks:
# 6.1.1 Test Siddon's algorithm with known cases
# 6.1.2 Test each reconstruction algorithm with synthetic data
# 6.1.3 Test phantom generation accuracy
# 6.1.4 Test noise injection statistics
# 6.1.5 Test metrics calculations against reference
# 6.1.6 Achieve >80% code coverage
```

---

#### Module 6.2: Performance Optimization
**File:** `optimization/`

```python
# Subtasks:
# 6.2.1 Profile code with cProfile
# 6.2.2 Optimize system matrix construction (Numba JIT)
# 6.2.3 Implement sparse matrix operations efficiently
# 6.2.4 Add multiprocessing for independent computations
# 6.2.5 Optional: GPU acceleration with CuPy
# 6.2.6 Memory profiling and optimization
# 6.2.7 Benchmark different algorithms
```

---

#### Module 6.3: Documentation
**File:** `docs/`

```python
# Subtasks:
# 6.3.1 Write API documentation (Sphinx)
# 6.3.2 Create user guide
# 6.3.3 Document installation process
# 6.3.4 Add example notebooks
# 6.3.5 Create troubleshooting guide
```

---

## 📦 Recommended Python Dependencies

```txt
# Core numerical computing
numpy>=1.24.0
scipy>=1.10.0

# Image processing
pillow>=10.0.0
scikit-image>=0.21.0

# Visualization
matplotlib>=3.7.0
pyqtgraph>=0.13.0  # Alternative: plotly

# GUI (choose one)
PyQt6>=6.5.0       # Option A: Full-featured desktop
# OR
streamlit>=1.28.0  # Option B: Web-based
# OR
# tkinter (built-in)  # Option C: Simple desktop

# Performance
numba>=0.57.0      # JIT compilation
joblib>=1.3.0      # Parallel processing

# Optional GPU
cupy-cuda12x>=12.0.0  # CUDA GPU acceleration

# Testing
pytest>=7.4.0
pytest-cov>=4.1.0

# Documentation
sphinx>=7.0.0
sphinx-rtd-theme>=1.3.0
```

---

## 🚀 Quick Start (Python Version)

```bash
# Clone repository
git clone https://github.com/yourusername/ct-simulator-python.git
cd ct-simulator-python

# Create virtual environment
python -m venv venv
source venv/bin/activate  # On Windows: venv\Scripts\activate

# Install dependencies
pip install -r requirements.txt

# Run the application
python main.py
```

---

## 📊 Comparison: C++ vs Python Implementation

| Aspect | C++ (Original) | Python (Proposed) |
|--------|---------------|-------------------|
| **Performance** | Excellent (native speed) | Good (with NumPy/Numba) |
| **Development Speed** | Slower (compilation, memory mgmt) | Faster (interpreted, concise) |
| **Memory Safety** | Manual (smart pointers help) | Automatic (garbage collection) |
| **GUI Framework** | Qt6/QML (native, powerful) | PyQt6/Streamlit (flexible) |
| **Threading** | QThread, QtConcurrent | threading, asyncio, multiprocessing |
| **Visualization** | QPainter, QQuickPaintedItem | matplotlib, pyqtgraph |
| **Learning Curve** | Steep (C++ expertise needed) | Gentle (accessible to students) |
| **Deployment** | Compiled binary | Python interpreter required |
| **Extensibility** | Moderate | High (rich ecosystem) |

---

## 📝 License

MIT License - See LICENSE file for details.

Original C++ implementation by Sanghyeb (Sam) Lee (2013), modernized 2024.

---

## 🤝 Contributing

Contributions are welcome! Please read our contributing guidelines before submitting pull requests.

---

## 📧 Contact

For questions or suggestions, please open an issue on GitHub.
