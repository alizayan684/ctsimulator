# CT Simulator Modernization - Team Task Breakdown
This software allow you to perform (1) X-ray CT acquisition, (2) System matrix generation va siddon's algorithm, (3) Sinogram generation, (4) Image reconstruction with FBP and SART.
## Project Overview
Reimplement the CTSimulator educational tool by:
1. Adding missing reconstruction algorithms (FBP, MLEM)
3. Building a modern, intuitive frontend
4. Creating interactive educational tutorials
5. Implementing advanced visualization features

---

## TASK 1: Core Reconstruction Algorithms

### Primary Responsibility
Implement the missing reconstruction algorithms and create a unified algorithm framework.

### Key Deliverables
- **FBP (Filtered Back Projection)**
  - Ram-Lak filter
  - Shepp-Logan filter
  - Cosine filter
  - Hamming filter
  - Hann filter
  
- **MLEM (Maximum Likelihood Expectation Maximization)**
  - Poisson noise model support
  - Configurable iterations
  - Convergence monitoring

- **Algorithm Framework**
  - Abstract base class for all reconstruction algorithms
  - Unified interface for ART, SIRT, FBP, MLEM
  - Progress callback system
  - Thread-safe execution

### Files to Create/Modify
```
src/reconstruction/
├── ReconstructionAlgorithm.h      # Abstract base class
├── ReconstructionAlgorithm.cpp
├── FBP.cpp                        # NEW
├── FBP.h
├── MLEM.cpp                       # NEW
├── MLEM.h
├── ART.cpp                        # Refactor existing
├── ART.h                          # Refactor existing
├── SIRT.cpp                       # Refactor existing
└── SIRT.h                         # Refactor existing
```

### Deliverables
- ✅ All 4 algorithms produce valid reconstructions
- ✅ Clean API for frontend integration

---

## TASK 2: Framework Migration & Modernization

### Primary Responsibility
Migrate from deprecated Qt4 to Qt6 and modernize the C++ codebase.

### Key Deliverables
- **Qt4 → Qt6 Migration**
  - Update all Qt includes and APIs
  - Replace deprecated signals/slots syntax
  - Migrate QPainter to modern rendering 

- **C++ Modernization (C++17/20)**
  - Replace raw pointers with smart pointers (`std::unique_ptr`, `std::shared_ptr`)
  - Use `std::vector` instead of raw arrays
  - Implement RAII patterns throughout
  - Add `constexpr` where applicable
  - Use structured bindings and auto type deduction

- **Memory Safety**
  - Eliminate all manual `new`/`delete`
  - Fix identified memory leaks
  - Add bounds checking
  - Implement proper exception handling


### Success Metrics
- ✅ All existing functionality preserved
- ✅ Modern C++ best practices throughout

---

##  TASK 3: Modern Frontend & User Experience

### Primary Responsibility
Design and implement a modern, intuitive user interface with excellent UX.

### Deliverables
- **UI Framework Migration**
  - Replace Qt Widgets with Qt Quick/QML
  - Responsive design 

- **Main Interface Components**
  - Phantom selection panel (preset + custom upload)
  - Acquisition parameter controls (sliders, spinboxes)
  - Algorithm selection and configuration panel
  - Real-time progress indicators
  - Multi-view layout manager

- **Data Management**
  - Export reconstructed images (PNG, TIFF)
  - Export sinograms
  - Save/Load session configurations (JSON)

- **User Workflow**
  - Undo/Redo functionality
  - Parameter presets
  - Quick comparison views (original vs reconstructed)

### Success Metrics
- ✅ Responsive UI (no freezing during computation)
- ✅ Professional, modern appearance

---

## TASK 4: Advanced Visualization & Educational Tutorials

### Primary Responsibility
Create advanced visualization features and interactive educational content.

### Key Deliverables
- **Advanced Visualization**
  - High-quality image rendering with colormaps
  - Zoom/Pan with smooth interpolation
  - ROI (Region of Interest) analysis tools
  - Line profiles and histogram displays
  - Side-by-side comparison mode
  - Difference image visualization
  - Error metrics display (RMSE, SSIM, PSNR)

- **System Matrix Visualization**
  - Interactive ray path display
  - Sparse matrix viewer
  - Animation of acquisition process

- **Interactive Tutorials**
  - **Tutorial 1**: "Introduction to CT" 
    - Basic principles explained with animations
  - **Tutorial 2**: "How Projections Work"
    - Interactive sinogram generation demo
  - **Tutorial 3**: "Reconstruction Algorithms Compared"
    - Side-by-side algorithm comparison with parameters
  - **Tutorial 4**: "Noise and Artifacts"
    - Demonstrate effects of noise, limited angles, etc.
  - **Tutorial 5**: "Parameter Optimization"
    - Guide users to optimal settings

- **Educational Features**
  - Tooltips with physics explanations
  - Glossary of CT terms


### Success Metrics
- ✅ All visualization tools functional and responsive
- ✅ 5 complete interactive tutorials
- ✅ Error metrics match reference implementations
- ✅ Smooth zoom/pan at high magnification
