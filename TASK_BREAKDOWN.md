# CT Simulator Modernization - Team Task Breakdown

## Project Overview
Reimplement the CTSimulator educational tool by:
1. Adding missing reconstruction algorithms (FBP, MLEM)
2. Replacing deprecated Qt4 with modern Qt6
3. Building a modern, intuitive frontend
4. Creating interactive educational tutorials
5. Implementing advanced visualization features

---

## 🎯 Member 1: Core Reconstruction Algorithms

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

### Integration Points
- Work with Member 2 on threading model
- Work with Member 3 on algorithm selection UI
- Work with Member 4 on visualization hooks

### Success Metrics
- ✅ All 4 algorithms produce valid reconstructions
- ✅ Unit tests pass for each algorithm
- ✅ Performance within 2x of original implementation
- ✅ Clean API for frontend integration

---

## 🎯 Member 2: Framework Migration & Modernization

### Primary Responsibility
Migrate from deprecated Qt4 to Qt6 and modernize the C++ codebase.

### Key Deliverables
- **Qt4 → Qt6 Migration**
  - Update all Qt includes and APIs
  - Replace deprecated signals/slots syntax
  - Migrate QPainter to modern rendering (coordinate with Member 4)
  - Update build system from qmake to CMake

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

- **Build System**
  - Cross-platform CMake configuration
  - CI/CD pipeline setup (GitHub Actions)
  - Package dependencies management (vcpkg or Conan)

### Files to Modify
```
All existing .h and .cpp files need modernization:
src/*.h, src/*.cpp
src/main.cpp
CMakeLists.txt                    # NEW (replace .pro files)
.clang-format                     # NEW
.gitignore                        # Update
```

### Integration Points
- Coordinate with Member 1 on algorithm interface design
- Coordinate with Member 3 on QML/C++ integration
- Coordinate with Member 4 on rendering backend

### Success Metrics
- ✅ Zero compiler warnings with `-Wall -Wextra -Wpedantic`
- ✅ No memory leaks (verified with Valgrind/ASan)
- ✅ Builds on Windows, macOS, and Linux
- ✅ All existing functionality preserved
- ✅ Modern C++ best practices throughout

---

## 🎯 Member 3: Modern Frontend & User Experience

### Primary Responsibility
Design and implement a modern, intuitive user interface with excellent UX.

### Key Deliverables
- **UI Framework Migration**
  - Replace Qt Widgets with Qt Quick/QML
  - Responsive design for different screen sizes
  - Dark/Light theme support
  - Accessible UI (keyboard navigation, screen readers)

- **Main Interface Components**
  - Phantom selection panel (preset + custom upload)
  - Acquisition parameter controls (sliders, spinboxes)
  - Algorithm selection and configuration panel
  - Real-time progress indicators
  - Multi-view layout manager

- **Data Management**
  - Export reconstructed images (PNG, TIFF, DICOM)
  - Export sinograms
  - Save/Load session configurations (JSON)
  - Batch processing support

- **User Workflow**
  - Guided step-by-step workflow
  - Undo/Redo functionality
  - Parameter presets
  - Quick comparison views (original vs reconstructed)

### Files to Create
```
qml/
├── Main.qml                       # Application root
├── components/
│   ├── PhantomSelector.qml
│   ├── AcquisitionControls.qml
│   ├── AlgorithmPanel.qml
│   ├── ReconstructionView.qml
│   ├── SinogramView.qml
│   ├── ProgressBar.qml
│   └── ThemeManager.qml
├── dialogs/
│   ├── ExportDialog.qml
│   ├── SettingsDialog.qml
│   └── AboutDialog.qml
└── layouts/
    ├── DefaultLayout.qml
    └── ComparisonLayout.qml

src/frontend/
├── MainWindow.h
├── MainWindow.cpp
├── ReconstructionController.h     # Bridge between QML and C++
├── ReconstructionController.cpp
└── ImageExporter.h
└── ImageExporter.cpp
```

### Integration Points
- Work with Member 1 on algorithm parameter exposure
- Work with Member 2 on QML/C++ integration patterns
- Work with Member 4 on visualization component integration

### Success Metrics
- ✅ Intuitive workflow (new users complete reconstruction in <2 min)
- ✅ All parameters accessible and well-documented
- ✅ Smooth animations (60 FPS)
- ✅ Responsive UI (no freezing during computation)
- ✅ Professional, modern appearance

---

## 🎯 Member 4: Advanced Visualization & Educational Tutorials

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
  - "What-if" scenario builder
  - Quiz mode for self-assessment
  - Glossary of CT terms

### Files to Create
```
src/visualization/
├── ImageViewer.h
├── ImageViewer.cpp
├── ColormapManager.h
├── ColormapManager.cpp
├── ROITools.h
├── ROITools.cpp
├── MetricsCalculator.h        # RMSE, SSIM, PSNR
├── MetricsCalculator.cpp
└── RayPathVisualizer.h
└── RayPathVisualizer.cpp

qml/tutorials/
├── TutorialManager.qml
├── IntroductionTutorial.qml
├── ProjectionsTutorial.qml
├── AlgorithmsTutorial.qml
├── NoiseTutorial.qml
└── OptimizationTutorial.qml

resources/
├── colormaps/                 # Viridis, Plasma, etc.
├── tutorial_assets/
└── icons/
```

### Integration Points
- Work with Member 1 on algorithm iteration visualization
- Work with Member 2 on rendering performance
- Work with Member 3 on tutorial UI integration

### Success Metrics
- ✅ All visualization tools functional and responsive
- ✅ 5 complete interactive tutorials
- ✅ Error metrics match reference implementations
- ✅ Smooth zoom/pan at high magnification
- ✅ Educational value validated by target users

---

## 📅 Suggested Timeline (8 Weeks)

### Week 1-2: Foundation
- **Member 1**: Design algorithm framework, start FBP
- **Member 2**: Set up CMake, begin Qt6 migration
- **Member 3**: Design UI mockups, set up QML project
- **Member 4**: Research visualization libraries, design tutorial content

### Week 3-4: Core Implementation
- **Member 1**: Complete FBP, start MLEM
- **Member 2**: Complete Qt6 migration, modernize core classes
- **Member 3**: Implement main UI components
- **Member 4**: Build basic visualization components

### Week 5-6: Integration
- **Member 1**: Complete MLEM, integrate all algorithms
- **Member 2**: Fix memory issues, set up CI/CD
- **Member 3**: Connect UI to backend, add export features
- **Member 4**: Complete visualization tools, start tutorials

### Week 7-8: Polish & Testing
- **All Members**: 
  - Integration testing
  - Complete tutorials
  - Bug fixes
  - Documentation
  - User testing and feedback

---

## 🔧 Recommended Technology Stack

| Component | Technology |
|-----------|------------|
| Framework | Qt 6.6+ LTS |
| Language | C++17 or C++20 |
| UI | QML with Qt Quick Controls 2 |
| Build | CMake 3.20+ |
| Rendering | Qt Quick Scene Graph (or OpenGL for Member 4) |
| Testing | Google Test + Qt Test |
| CI/CD | GitHub Actions |
| Documentation | Doxygen + Markdown |
| Package Manager | vcpkg or Conan |

---

## 📋 Weekly Sync Agenda

1. **Progress Updates** (each member, 5 min)
2. **Integration Issues** (blocking problems)
3. **API Changes** (affecting other members)
4. **Next Week Goals** (specific deliverables)
5. **Open Discussion** (ideas, concerns)

---

## ✅ Definition of Done

A feature is considered complete when:
- [ ] Code is implemented and follows team standards
- [ ] Unit tests written and passing
- [ ] Integrated with other components
- [ ] Documented (code comments + user docs)
- [ ] Tested on all target platforms
- [ ] Reviewed by at least one other team member
- [ ] No memory leaks or performance regressions

---

## 📚 Additional Resources

- **Qt6 Documentation**: https://doc.qt.io/qt-6/
- **QML Best Practices**: https://doc.qt.io/qt-6/qml-best-practices.html
- **CT Reconstruction Theory**: Kak & Slaney, "Principles of Computerized Tomographic Imaging"
- **Modern C++ Guidelines**: https://isocpp.github.io/CppCoreGuidelines/

---

*This breakdown ensures clear ownership while maintaining necessary collaboration points for a cohesive final product.*
