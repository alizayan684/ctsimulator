import sys
import os
import json
import numpy as np
import scipy.sparse as sp
import scipy.sparse.linalg
import scipy.fft
import scipy.ndimage
from PyQt6.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout, 
                             QHBoxLayout, QLabel, QSlider, QComboBox, QPushButton, 
                             QGroupBox, QFormLayout, QTabWidget, QListWidget, 
                             QTextBrowser, QFileDialog, QMessageBox, QSpinBox,
                             QDoubleSpinBox, QProgressBar, QSplitter)
from PyQt6.QtCore import Qt, QThread, pyqtSignal, QObject
from PyQt6.QtGui import QFont, QAction

import matplotlib
matplotlib.use('QtAgg')
from matplotlib.backends.backend_qtagg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.figure import Figure
import matplotlib.image as mpimg
from matplotlib.lines import Line2D

# ==============================================================================
# CORE MATH & CT SIMULATION ENGINE
# ==============================================================================

class CTMathEngine:

    @staticmethod
    def generate_shepp_logan(N):
        """Generates a standard Shepp-Logan phantom image using analytical ellipses."""
        ellipses = [
            [2.0,   0.69,   0.92,   0.0,    0.0,     0.0],
            [-0.8,  0.6624, 0.8740, 0.0,   -0.0184,  0.0],
            [-0.2,  0.1100, 0.3100, 0.22,   0.0,    -18.0],
            [-0.2,  0.1600, 0.4100, -0.22,  0.0,     18.0],
            [0.1,   0.0460, 0.0460, 0.0,    0.35,    0.0],
            [0.1,   0.0460, 0.0460, 0.0,    0.1,     0.0],
            [0.1,   0.0230, 0.0230, -0.08, -0.605,   0.0],
            [0.1,   0.0230, 0.0230, 0.0,   -0.605,   0.0],
            [0.1,   0.0460, 0.0230, 0.06,  -0.605,   90.0]
        ]
        
        x = np.linspace(-1, 1, N)
        y = np.linspace(1, -1, N)
        X, Y = np.meshgrid(x, y)
        phantom = np.zeros((N, N))
        
        for intensity, a, b, x0, y0, phi in ellipses:
            phi_rad = np.radians(phi)
            cos_p, sin_p = np.cos(phi_rad), np.sin(phi_rad)
            X_rot = (X - x0) * cos_p + (Y - y0) * sin_p
            Y_rot = -(X - x0) * sin_p + (Y - y0) * cos_p
            mask = (X_rot**2 / a**2 + Y_rot**2 / b**2) <= 1.0
            phantom[mask] += intensity
            
        phantom = np.clip(phantom, 0, None)
        if phantom.max() > 0:
            phantom /= phantom.max()
        return phantom

    @staticmethod
    def load_phantom_from_file(filepath, N):
        """Loads a custom image or .txt array, converts to grayscale, and interpolates to NxN grid."""
        try:
            if filepath.lower().endswith('.txt') or filepath.lower().endswith('.csv'):
                try:
                    img = np.loadtxt(filepath, delimiter=',')
                except ValueError:
                    img = np.loadtxt(filepath)
            else:
                img = mpimg.imread(filepath)
                # Convert to grayscale if RGB/RGBA
                if len(img.shape) == 3:
                    img = np.dot(img[..., :3], [0.2989, 0.5870, 0.1140])
                
            # Resize using scipy's zoom
            zoom_factor = (N / img.shape[0], N / img.shape[1])
            phantom = scipy.ndimage.zoom(img, zoom_factor, order=1)
            
            # Normalize
            phantom = np.clip(phantom, 0, None)
            if phantom.max() > 0:
                phantom /= phantom.max()
            return phantom
        except Exception as e:
            raise RuntimeError(f"Failed to load phantom: {str(e)}")

    @staticmethod
    def generate_system_matrix_exact(N, num_angles, num_detectors, progress_callback=None, update_callback=None):
        """
        Calculates the exact chord lengths of 2D rays intersecting square pixels.
        This provides exact Siddon-level accuracy utilizing an analytical footprint model.
        """
        X, Y = np.meshgrid(np.linspace(-1, 1, N), np.linspace(1, -1, N))
        X_flat = X.flatten()
        Y_flat = Y.flatten()
        
        theta = np.linspace(0, np.pi, num_angles, endpoint=False)
        det_pos = np.linspace(-1.5, 1.5, num_detectors)
        h = 2.0 / N  # Pixel width
        
        row_list, col_list, data_list = [], [], []
        
        for i, t in enumerate(theta):
            if progress_callback and i % max(1, num_angles // 10) == 0:
                progress_callback(int((i / num_angles) * 100))
                
            cos_t, sin_t = np.cos(t), np.sin(t)
            c, s_ = np.abs(cos_t), np.abs(sin_t)
            
            # Precompute intersection profile boundaries for a pixel
            w1 = (h / 2.0) * np.abs(c - s_)
            w2 = (h / 2.0) * (c + s_)
            L_max = h / max(c, s_) if max(c, s_) > 1e-6 else h
            
            # Distance of each pixel center to the unshifted ray axis
            proj_coords = X_flat * cos_t + Y_flat * sin_t
            
            for j, s in enumerate(det_pos):
                d = np.abs(proj_coords - s)
                mask = d < w2
                cols = np.where(mask)[0]
                
                if len(cols) > 0:
                    d_sub = d[cols]
                    weights = np.zeros_like(d_sub)
                    
                    # Core Analytical Exact Intersect Profile (Siddon equivalence)
                    m1 = d_sub <= w1
                    weights[m1] = L_max
                    
                    m2 = (d_sub > w1) & (d_sub < w2)
                    if c * s_ > 1e-6:
                        weights[m2] = (w2 - d_sub[m2]) / (c * s_)
                    else:
                        weights[m2] = L_max * (w2 - d_sub[m2]) / (w2 - w1)
                        
                    row_list.extend([i * num_detectors + j] * len(cols))
                    col_list.extend(cols)
                    data_list.extend(weights)

        A = sp.coo_matrix((data_list, (row_list, col_list)), 
                          shape=(num_angles * num_detectors, N * N)).tocsr()
        
        if progress_callback:
            progress_callback(100)
        return A

    @staticmethod
    def simulate_acquisition_physics(sinogram_clean, mA, kVp):
        """
        Simulates physics of acquisition including Polychromatic Beam Hardening
        and true Poisson noise statistics.
        """
        # Baseline photon count (I0) scales with mA and kVp^2
        I0_total = 1000 * mA * (kVp / 100.0)**2
        
        # Simulate a polychromatic spectrum using 3 energy bins
        # Weights represent the spectrum distribution, mu_multipliers represent how
        # much stronger lower energies are attenuated (higher mu).
        weights = [0.2, 0.5, 0.3]
        mu_multipliers = [1.5, 1.0, 0.8] 
        
        I_mean = np.zeros_like(sinogram_clean)
        
        # Sum the attenuated intensities across the different energy bins
        for w, mu_mult in zip(weights, mu_multipliers):
            I_mean += (w * I0_total) * np.exp(-sinogram_clean * mu_mult)
        
        # Apply True Poisson Noise
        # Poisson generation directly models the quantum statistics of X-ray detection
        I_noisy = np.random.poisson(I_mean)
        I_noisy = I_noisy.astype(np.float64)
        I_noisy[I_noisy == 0] = 1.0 # Minimum 1 photon to avoid log(0) for totally blocked rays
        
        # Convert back to attenuation sinogram
        # Note: We NO LONGER clip negative values to 0. Real noise allows I_noisy > I0_total 
        # which results in negative attenuation. Clipping biases the data.
        sinogram_noisy = -np.log(I_noisy / I0_total)
        return sinogram_noisy


# ==============================================================================
# RECONSTRUCTION ALGORITHMS
# ==============================================================================

class ReconAlgorithms:
    
    @staticmethod
    def ART(A, b, shape, iterations, relaxation, tol, progress_callback=None, update_callback=None):
        """Algebraic Reconstruction Technique with under-relaxation."""
        x = np.zeros(A.shape[1])
        row_norms_sq = np.array(A.multiply(A).sum(axis=1)).flatten() + 1e-8
        
        for it in range(iterations):
            if progress_callback: progress_callback(int((it / iterations) * 100))
            indices = np.random.permutation(A.shape[0])
            
            for idx in indices:
                row = A.getrow(idx)
                a_i = row.toarray().flatten()
                proj = np.dot(a_i, x)
                error = b[idx] - proj
                x += relaxation * (error / row_norms_sq[idx]) * a_i
                
            x = np.maximum(x, 0)  # Positivity constraint
            
            if update_callback:
                update_callback(x.copy().reshape(shape))
                
        return x.reshape(shape)

    @staticmethod
    def SIRT(A, b, shape, iterations, relaxation, tol, progress_callback=None, update_callback=None):
        """Simultaneous Iterative Reconstruction Technique."""
        x = np.zeros(A.shape[1])
        A_sum = np.array(A.sum(axis=0)).flatten() + 1e-8
        A_T_sum = np.array(A.sum(axis=1)).flatten() + 1e-8
        
        for it in range(iterations):
            if progress_callback: progress_callback(int((it / iterations) * 100))
            
            proj = A.dot(x)
            diff = b - proj
            update = A.T.dot(diff / A_T_sum)
            x += relaxation * (update / A_sum)
            x = np.maximum(x, 0)
            
            if update_callback:
                update_callback(x.copy().reshape(shape))
                
        return x.reshape(shape)

    @staticmethod
    def TR_ML(A, b, shape, iterations, relaxation, tol, progress_callback=None, update_callback=None):
        """
        Transmission Maximum Likelihood using Separable Paraboloidal Surrogates (SPS).
        Correctly maximizes the Poisson likelihood for transmission statistics.
        """
        # Start with a uniform positive image (e.g., water-like attenuation)
        x = np.ones(A.shape[1]) * 0.01 
        
        # Measured normalized transmission counts (Y_meas = I/I0 = e^-b)
        y_meas = np.exp(-b)
        
        # Precompute the row sums of the system matrix (used in the Hessian surrogate)
        A_row_sums = np.array(A.sum(axis=1)).flatten()
        
        for it in range(iterations):
            if progress_callback: progress_callback(int((it / iterations) * 100))
            
            # 1. Forward project current image to get estimated transmission counts
            proj = A.dot(x)
            y_est = np.exp(-proj)
            
            # 2. Gradient of the Transmission Log-Likelihood
            # Notice this is y_est - y_meas. If y_est > y_meas (we underestimate attenuation), 
            # the gradient is positive, forcing 'x' to increase.
            grad = A.T.dot(y_est - y_meas)
            
            # 3. Separable Surrogate Hessian (Denominator for stability)
            denom = A.T.dot(y_est * A_row_sums)
            denom[denom == 0] = 1e-8  # Prevent division by zero
            
            # 4. Additive update step (Newton-Raphson style)
            x = x + relaxation * (grad / denom)
            
            # Positivity constraint (attenuation cannot be negative)
            x = np.maximum(x, 0) 
            
            if update_callback:
                update_callback(x.copy().reshape(shape))
                
        return x.reshape(shape)
    @staticmethod
    def FBP(b, shape, num_angles, num_detectors, filter_type, progress_callback=None, update_callback=None):
        """
        True Filtered Back Projection utilizing sub-pixel parallel beam backprojection.
        Includes correct dimensional scaling to accurately calculate attenuation coefficients.
        """
        if progress_callback: progress_callback(10)
        sinogram = b.reshape((num_angles, num_detectors))
        
        # 1. Filtration in frequency domain
        pad_len = 2 ** int(np.ceil(np.log2(num_detectors * 2)))
        freqs = scipy.fft.fftfreq(pad_len)
        omega = 2 * np.pi * freqs
        ramp = np.abs(freqs) * 2.0
        
        if filter_type == 'Shepp-Logan':
            sinc_val = np.sinc(omega / (2 * np.pi))
            filt = ramp * sinc_val
        elif filter_type == 'Cosine':
            filt = ramp * np.cos(omega / 4.0)
        elif filter_type == 'Hamming':
            filt = ramp * (0.54 + 0.46 * np.cos(omega / 2.0))
        elif filter_type == 'Hann':
            filt = ramp * (0.5 + 0.5 * np.cos(omega / 2.0))
        else:
            filt = ramp # Ram-Lak fallback
            
        filtered_sino = np.zeros_like(sinogram)
        for i in range(num_angles):
            proj = sinogram[i, :]
            proj_fft = scipy.fft.fft(proj, n=pad_len)
            filtered_proj = scipy.fft.ifft(proj_fft * filt).real
            filtered_sino[i, :] = filtered_proj[:num_detectors]
            
        if progress_callback: progress_callback(30)
        
        # 2. True Continuous Backprojection
        N = shape[0]
        theta = np.linspace(0, np.pi, num_angles, endpoint=False)
        det_pos = np.linspace(-1.5, 1.5, num_detectors)
        ds = 3.0 / num_detectors  # The spatial pitch of the detector array
        
        X, Y = np.meshgrid(np.linspace(-1, 1, N), np.linspace(1, -1, N))
        X_flat = X.flatten()
        Y_flat = Y.flatten()
        x_recon = np.zeros(N * N)
        
        update_freq = max(1, num_angles // 20)
        
        for i, t in enumerate(theta):
            if progress_callback and i % max(1, num_angles // 10) == 0:
                progress_callback(30 + int((i / num_angles) * 70))
                
            s_grid = X_flat * np.cos(t) + Y_flat * np.sin(t)
            proj_interp = np.interp(s_grid, det_pos, filtered_sino[i, :], left=0, right=0)
            x_recon += proj_interp
            
            if update_callback and i > 0 and (i % update_freq == 0):
                # Temporary scaling for progressive UI updates
                temp_recon = x_recon * (np.pi / (i + 1)) * ds
                update_callback(temp_recon.reshape(shape))
            
        # Final mathematical scaling by (pi / N_angles) * ds to restore physical units
        x_recon *= (np.pi / num_angles) * ds
        
        if progress_callback: progress_callback(100)
        return x_recon.reshape(shape)


# ==============================================================================
# METRICS CALCULATOR
# ==============================================================================

class Metrics:
    @staticmethod
    def rmse(ref, target):
        return np.sqrt(np.mean((ref - target)**2))
        
    @staticmethod
    def psnr(ref, target):
        mse = np.mean((ref - target)**2)
        if mse == 0: return float('inf')
        return 20 * np.log10(1.0 / np.sqrt(mse))


# ==============================================================================
# WORKER THREAD FOR BACKGROUND PROCESSING
# ==============================================================================

class WorkerSignals(QObject):
    progress = pyqtSignal(int)
    intermediate = pyqtSignal(object)
    finished = pyqtSignal(object)
    error = pyqtSignal(str)

class Worker(QThread):
    def __init__(self, fn, *args, **kwargs):
        super().__init__()
        self.fn = fn
        self.args = args
        self.kwargs = kwargs
        self.signals = WorkerSignals()

    def run(self):
        try:
            self.kwargs['progress_callback'] = self.signals.progress.emit
            self.kwargs['update_callback'] = self.signals.intermediate.emit
            result = self.fn(*self.args, **self.kwargs)
            self.signals.finished.emit(result)
        except Exception as e:
            import traceback
            traceback.print_exc()
            self.signals.error.emit(str(e))


# ==============================================================================
# GUI APPLICATION
# ==============================================================================

class CTApp(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("CT Simulator Pro - Educational Platform")
        self.setGeometry(50, 50, 1380, 850)
        
        self.phantom = None
        self.system_matrix = None
        self.sinogram = None
        self.reconstruction = None
        
        self.current_N = 64
        self.current_angles = 90
        self.current_dets = 90
        
        self.line_drawing = False
        self.profile_line = None
        self.line_points = []
        
        self.init_ui()
        self.generate_matrix_and_phantom()

    def init_ui(self):
        main_widget = QWidget()
        self.setCentralWidget(main_widget)
        main_layout = QHBoxLayout(main_widget)
        
        splitter = QSplitter(Qt.Orientation.Horizontal)
        main_layout.addWidget(splitter)
        
        # LEFT PANEL: Controls
        control_panel = QWidget()
        control_layout = QVBoxLayout(control_panel)
        control_layout.setAlignment(Qt.AlignmentFlag.AlignTop)
        
        # Geometry Settings
        geom_group = QGroupBox("1. Geometry & System")
        geom_layout = QFormLayout()
        
        self.spin_grid = QSpinBox()
        self.spin_grid.setRange(16, 256); self.spin_grid.setValue(64)
        
        self.spin_angles = QSpinBox()
        self.spin_angles.setRange(10, 720); self.spin_angles.setValue(90)
        
        self.spin_detectors = QSpinBox()
        self.spin_detectors.setRange(16, 512); self.spin_detectors.setValue(90)
        
        self.spin_grid.valueChanged.connect(self.invalidate_matrix)
        self.spin_angles.valueChanged.connect(self.invalidate_matrix)
        self.spin_detectors.valueChanged.connect(self.invalidate_matrix)
        
        self.btn_load_phantom = QPushButton("Load Custom Phantom (.txt/Img)")
        self.btn_load_phantom.clicked.connect(self.load_custom_phantom)
        self.btn_generate_sys = QPushButton("Generate Shepp-Logan & Matrix")
        self.btn_generate_sys.clicked.connect(self.generate_matrix_and_phantom)
        self.btn_generate_sys.setStyleSheet("background-color: #2196F3; color: white;")
        
        geom_layout.addRow("Grid Size:", self.spin_grid)
        geom_layout.addRow("Projections:", self.spin_angles)
        geom_layout.addRow("Detectors:", self.spin_detectors)
        geom_layout.addRow(self.btn_load_phantom)
        geom_layout.addRow(self.btn_generate_sys)
        geom_group.setLayout(geom_layout)
        control_layout.addWidget(geom_group)
        
        # Acquisition Settings
        acq_group = QGroupBox("2. Physics Acquisition")
        acq_layout = QFormLayout()
        
        self.spin_ma = QSpinBox()
        self.spin_ma.setRange(1, 1000); self.spin_ma.setValue(200)
        self.spin_ma.setToolTip("Tube Current (mA). Lower = more Poisson noise.")
        
        self.spin_kvp = QSpinBox()
        self.spin_kvp.setRange(40, 150); self.spin_kvp.setValue(120)
        
        self.btn_generate_sino = QPushButton("Simulate Acquisition")
        self.btn_generate_sino.clicked.connect(self.simulate_acquisition)
        
        acq_layout.addRow("Tube Current (mA):", self.spin_ma)
        acq_layout.addRow("Tube Voltage (kVp):", self.spin_kvp)
        acq_layout.addRow(self.btn_generate_sino)
        acq_group.setLayout(acq_layout)
        control_layout.addWidget(acq_group)
        
        # Reconstruction Settings
        recon_group = QGroupBox("3. Reconstruction")
        recon_layout = QFormLayout()
        
        self.combo_algo = QComboBox()
        self.combo_algo.addItems(["FBP", "ART", "SIRT", "TR-ML (Transmission ML)"])
        self.combo_algo.currentTextChanged.connect(self.toggle_algo_params)
        
        self.spin_iters = QSpinBox()
        self.spin_iters.setRange(1, 1000); self.spin_iters.setValue(20)
        
        self.spin_relax = QDoubleSpinBox()
        self.spin_relax.setRange(0.01, 2.0); self.spin_relax.setValue(0.1); self.spin_relax.setSingleStep(0.1)
        
        self.spin_tol = QDoubleSpinBox()
        self.spin_tol.setRange(0.0001, 0.1); self.spin_tol.setValue(0.001); self.spin_tol.setDecimals(4)
        
        self.combo_filter = QComboBox()
        self.combo_filter.addItems(["Ram-Lak", "Shepp-Logan", "Cosine", "Hamming", "Hann"])
        
        self.btn_reconstruct = QPushButton("Reconstruct Image")
        self.btn_reconstruct.clicked.connect(self.run_reconstruction)
        self.btn_reconstruct.setStyleSheet("font-weight: bold; padding: 10px; background-color: #4CAF50; color: white;")
        
        recon_layout.addRow("Algorithm:", self.combo_algo)
        recon_layout.addRow("Iterations:", self.spin_iters)
        recon_layout.addRow("Relaxation:", self.spin_relax)
        recon_layout.addRow("Tol (Early Stop):", self.spin_tol)
        recon_layout.addRow("FBP Filter:", self.combo_filter)
        recon_layout.addRow(self.btn_reconstruct)
        recon_group.setLayout(recon_layout)
        control_layout.addWidget(recon_group)
        
        # View Settings
        view_group = QGroupBox("View Settings")
        view_layout = QFormLayout()
        self.combo_cmap = QComboBox()
        self.combo_cmap.addItems(["gray", "bone", "viridis", "inferno"])
        self.combo_cmap.currentTextChanged.connect(self.update_visualizations)
        view_layout.addRow("Colormap:", self.combo_cmap)
        view_group.setLayout(view_layout)
        control_layout.addWidget(view_group)
        
        self.progress_bar = QProgressBar()
        control_layout.addWidget(self.progress_bar)
        
        self.lbl_status = QLabel("Ready")
        self.lbl_status.setWordWrap(True)
        control_layout.addWidget(self.lbl_status)
        
        # RIGHT PANEL: Tabs
        self.tabs = QTabWidget()
        
        # Tab 1: Visualizer
        self.sim_tab = QWidget()
        sim_layout = QVBoxLayout(self.sim_tab)
        
        self.fig = Figure(figsize=(12, 4), tight_layout=True)
        self.canvas = FigureCanvas(self.fig)
        self.ax_phantom = self.fig.add_subplot(131)
        self.ax_sino = self.fig.add_subplot(132)
        self.ax_recon = self.fig.add_subplot(133)
        self.clear_axes()
        sim_layout.addWidget(self.canvas)
        
        self.lbl_metrics = QLabel("Metrics: N/A")
        self.lbl_metrics.setFont(QFont("Arial", 12, QFont.Weight.Bold))
        self.lbl_metrics.setAlignment(Qt.AlignmentFlag.AlignCenter)
        sim_layout.addWidget(self.lbl_metrics)
        self.tabs.addTab(self.sim_tab, "1. Process Visualizer")
        
        # Tab 2: ROI Analysis & Profiles
        self.analysis_tab = QWidget()
        analysis_layout = QHBoxLayout(self.analysis_tab)
        
        self.fig_analysis = Figure(figsize=(6, 5), tight_layout=True)
        self.canvas_analysis = FigureCanvas(self.fig_analysis)
        self.ax_analysis_img = self.fig_analysis.add_subplot(111)
        self.ax_analysis_img.set_title("Draw Line for Profile (Click & Drag)")
        self.ax_analysis_img.axis('off')
        
        self.canvas_analysis.mpl_connect('button_press_event', self.on_mouse_press)
        self.canvas_analysis.mpl_connect('motion_notify_event', self.on_mouse_drag)
        self.canvas_analysis.mpl_connect('button_release_event', self.on_mouse_release)
        analysis_layout.addWidget(self.canvas_analysis)
        
        right_analysis_layout = QVBoxLayout()
        self.fig_profile = Figure(figsize=(6, 4), tight_layout=True)
        self.canvas_profile = FigureCanvas(self.fig_profile)
        self.ax_profile = self.fig_profile.add_subplot(111)
        self.ax_profile.set_title("Line Profile")
        self.ax_profile.set_xlabel("Distance (pixels)")
        self.ax_profile.set_ylabel("Intensity")
        self.ax_profile.grid(True)
        
        self.lbl_roi_stats = QLabel("ROI Statistics:\nMean: --\nMin: --\nMax: --\nStd Dev: --")
        self.lbl_roi_stats.setFont(QFont("Monospace", 12))
        
        right_analysis_layout.addWidget(self.canvas_profile)
        right_analysis_layout.addWidget(self.lbl_roi_stats)
        analysis_layout.addLayout(right_analysis_layout)
        
        self.tabs.addTab(self.analysis_tab, "2. ROI & Analysis Tools")
        
        splitter.addWidget(control_panel)
        splitter.addWidget(self.tabs)
        splitter.setSizes([320, 1060])
        self.toggle_algo_params()

    def invalidate_matrix(self):
        self.system_matrix = None
        self.btn_generate_sino.setEnabled(False)
        self.btn_reconstruct.setEnabled(False)
        self.lbl_status.setText("Geometry changed. Matrix regeneration required! Click 'Generate'.")

    def toggle_algo_params(self):
        algo = self.combo_algo.currentText()
        is_iterative = algo in ["ART", "SIRT", "TR-ML (Transmission ML)"]
        self.spin_iters.setEnabled(is_iterative)
        self.spin_relax.setEnabled(algo in ["ART", "SIRT"])
        self.spin_tol.setEnabled(is_iterative)
        self.combo_filter.setEnabled(algo == "FBP")

    def clear_axes(self):
        self.ax_phantom.clear(); self.ax_phantom.set_title("1. Original Phantom")
        self.ax_sino.clear(); self.ax_sino.set_title("2. Sinogram (Acquisition)")
        self.ax_recon.clear(); self.ax_recon.set_title("3. Reconstruction")
        for ax in [self.ax_phantom, self.ax_sino, self.ax_recon]:
            ax.axis('off')
        self.canvas.draw()

    def set_gui_enabled(self, state):
        self.spin_grid.setEnabled(state)
        self.spin_angles.setEnabled(state)
        self.spin_detectors.setEnabled(state)
        self.btn_generate_sys.setEnabled(state)
        self.btn_load_phantom.setEnabled(state)
        
        if self.system_matrix is not None:
            self.btn_generate_sino.setEnabled(state)
        if self.sinogram is not None:
            self.btn_reconstruct.setEnabled(state)

    def load_custom_phantom(self):
        filepath, _ = QFileDialog.getOpenFileName(self, "Load Custom Phantom", "", "Text Files (*.txt *.csv);;Images (*.png *.jpg *.bmp *.tif);;All Files (*)")
        if filepath:
            N = self.spin_grid.value()
            try:
                self.phantom = CTMathEngine.load_phantom_from_file(filepath, N)
                self.update_visualizations()
                self.run_system_matrix_worker(N)
            except Exception as e:
                QMessageBox.critical(self, "Load Error", str(e))

    def generate_matrix_and_phantom(self):
        N = self.spin_grid.value()
        self.phantom = CTMathEngine.generate_shepp_logan(N)
        self.update_visualizations()
        self.run_system_matrix_worker(N)

    def run_system_matrix_worker(self, N):
        self.set_gui_enabled(False)
        self.lbl_status.setText("Calculating Exact Siddon Intersections...")
        self.progress_bar.setValue(0)
        
        self.current_N = N
        self.current_angles = self.spin_angles.value()
        self.current_dets = self.spin_detectors.value()
        
        self.sinogram = None
        self.reconstruction = None
        
        self.worker = Worker(CTMathEngine.generate_system_matrix_exact, self.current_N, self.current_angles, self.current_dets)
        self.worker.signals.progress.connect(self.progress_bar.setValue)
        self.worker.signals.finished.connect(self.on_matrix_generated)
        self.worker.signals.error.connect(self.on_error)
        self.worker.start()

    def on_matrix_generated(self, A):
        self.system_matrix = A
        self.lbl_status.setText("Exact System Matrix Ready.")
        self.progress_bar.setValue(100)
        self.set_gui_enabled(True)
        self.simulate_acquisition()

    def simulate_acquisition(self):
        if self.system_matrix is None or self.phantom is None:
            QMessageBox.warning(self, "Error", "Generate Matrix & Phantom first.")
            return
            
        self.lbl_status.setText("Simulating Physics Acquisition...")
        QApplication.processEvents()
        
        x_flat = self.phantom.flatten()
        b_clean = self.system_matrix.dot(x_flat)
        
        mA = self.spin_ma.value()
        kVp = self.spin_kvp.value()
        self.sinogram = CTMathEngine.simulate_acquisition_physics(b_clean, mA, kVp)
        
        self.reconstruction = None
        self.lbl_status.setText(f"Sinogram Generated (mA={mA}, kVp={kVp}).")
        self.update_visualizations()
        self.set_gui_enabled(True)

    def run_reconstruction(self):
        if self.sinogram is None:
            QMessageBox.warning(self, "Error", "Simulate Acquisition first.")
            return
            
        self.set_gui_enabled(False)
        self.lbl_status.setText("Reconstructing...")
        self.progress_bar.setValue(0)
        
        algo = self.combo_algo.currentText()
        iters = self.spin_iters.value()
        relax = self.spin_relax.value()
        tol = self.spin_tol.value()
        filt = self.combo_filter.currentText()
        shape = self.phantom.shape
        
        A = self.system_matrix
        b = self.sinogram
        
        if algo == "ART":
            self.worker = Worker(ReconAlgorithms.ART, A, b, shape, iters, relax, tol)
        elif algo == "SIRT":
            self.worker = Worker(ReconAlgorithms.SIRT, A, b, shape, iters, relax, tol)
        elif algo == "TR-ML (Transmission ML)":
            self.worker = Worker(ReconAlgorithms.TR_ML, A, b, shape, iters, relax, tol)
        elif algo == "FBP":
            self.worker = Worker(ReconAlgorithms.FBP, b, shape, self.current_angles, self.current_dets, filt)
            
        self.worker.signals.progress.connect(self.progress_bar.setValue)
        self.worker.signals.intermediate.connect(self.on_intermediate_reconstruction)
        self.worker.signals.finished.connect(self.on_reconstruction_finished)
        self.worker.signals.error.connect(self.on_error)
        self.worker.start()

    def on_intermediate_reconstruction(self, recon):
        self.reconstruction = recon
        if self.reconstruction.max() > 0:
            self.reconstruction = self.reconstruction / self.reconstruction.max()
        
        self.update_visualizations()
        self.calculate_metrics()

    def on_reconstruction_finished(self, recon):
        self.reconstruction = recon
        if self.reconstruction.max() > 0:
            self.reconstruction = self.reconstruction / self.reconstruction.max()
            
        self.lbl_status.setText("Reconstruction Complete.")
        self.progress_bar.setValue(100)
        self.set_gui_enabled(True)
        self.update_visualizations()
        self.calculate_metrics()

    def on_error(self, err_msg):
        QMessageBox.critical(self, "Processing Error", f"An error occurred:\n{err_msg}")
        self.lbl_status.setText("Error occurred.")
        self.set_gui_enabled(True)

    def update_visualizations(self):
        cmap = self.combo_cmap.currentText()
        self.clear_axes()
        
        if self.phantom is not None:
            self.ax_phantom.imshow(self.phantom, cmap=cmap, origin='upper', vmin=0, vmax=1)
            self.ax_phantom.set_title("1. Original Phantom")
            
        if self.sinogram is not None:
            sino_2d = self.sinogram.reshape((self.current_angles, self.current_dets))
            # Removing bounds since noise can create slightly negative values naturally
            self.ax_sino.imshow(sino_2d, cmap=cmap, aspect='auto', origin='upper')
            self.ax_sino.set_title("2. Sinogram (With Noise)")
            self.ax_sino.set_xlabel("Detector Position")
            self.ax_sino.set_ylabel("Projection Angle")
            
        if self.reconstruction is not None:
            self.ax_recon.imshow(self.reconstruction, cmap=cmap, origin='upper', vmin=0, vmax=1)
            self.ax_recon.set_title(f"3. Reconstruction ({self.combo_algo.currentText()})")
            
            self.ax_analysis_img.clear()
            self.ax_analysis_img.imshow(self.reconstruction, cmap=cmap, origin='upper', vmin=0, vmax=1)
            self.ax_analysis_img.set_title("Draw Line for Profile (Click & Drag)")
            self.ax_analysis_img.axis('off')
            if self.profile_line:
                self.ax_analysis_img.add_line(self.profile_line)
            self.canvas_analysis.draw()
            
        self.canvas.draw()

    def calculate_metrics(self):
        if self.phantom is not None and self.reconstruction is not None:
            rmse_val = Metrics.rmse(self.phantom, self.reconstruction)
            psnr_val = Metrics.psnr(self.phantom, self.reconstruction)
            self.lbl_metrics.setText(f"Metrics | RMSE: {rmse_val:.4f} | PSNR: {psnr_val:.2f} dB")

    # --- ROI & Line Profile Mouse Events ---
    def on_mouse_press(self, event):
        if event.inaxes != self.ax_analysis_img or self.reconstruction is None: return
        self.line_drawing = True
        self.line_points = [(event.xdata, event.ydata), (event.xdata, event.ydata)]
        if self.profile_line:
            self.profile_line.remove()
        self.profile_line = Line2D([event.xdata, event.xdata], [event.ydata, event.ydata], color='red', lw=2)
        self.ax_analysis_img.add_line(self.profile_line)
        self.canvas_analysis.draw()

    def on_mouse_drag(self, event):
        if not self.line_drawing or event.inaxes != self.ax_analysis_img: return
        self.line_points[1] = (event.xdata, event.ydata)
        self.profile_line.set_data([self.line_points[0][0], self.line_points[1][0]], 
                                   [self.line_points[0][1], self.line_points[1][1]])
        self.canvas_analysis.draw()

    def on_mouse_release(self, event):
        if not self.line_drawing: return
        self.line_drawing = False
        self.extract_line_profile()

    def extract_line_profile(self):
        if not self.line_points or self.reconstruction is None: return
        
        x0, y0 = self.line_points[0]
        x1, y1 = self.line_points[1]
        length = int(np.hypot(x1 - x0, y1 - y0))
        if length == 0: return
        
        x_idx, y_idx = np.linspace(x0, x1, length), np.linspace(y0, y1, length)
        zi = scipy.ndimage.map_coordinates(self.reconstruction, np.vstack((y_idx, x_idx)))
        
        self.ax_profile.clear()
        self.ax_profile.set_title("Line Profile")
        self.ax_profile.set_xlabel("Distance (pixels)")
        self.ax_profile.set_ylabel("Intensity")
        self.ax_profile.grid(True)
        self.ax_profile.plot(zi, color='red')
        self.canvas_profile.draw()
        
        mean_val, min_val, max_val, std_val = np.mean(zi), np.min(zi), np.max(zi), np.std(zi)
        self.lbl_roi_stats.setText(f"ROI Statistics:\nMean: {mean_val:.4f}\nMin: {min_val:.4f}\nMax: {max_val:.4f}\nStd Dev: {std_val:.4f}")

if __name__ == '__main__':
    sys.setrecursionlimit(5000)
    app = QApplication(sys.argv)
    app.setStyle("Fusion")
    window = CTApp()
    window.show()
    sys.exit(app.exec())