import sys
import os
import threading
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
import whisper
import srt
import datetime
import warnings
import subprocess
import imageio_ffmpeg
import numpy as np

# Suppress specific warnings
warnings.filterwarnings(
    "ignore", message="FP16 is not supported on CPU; using FP32 instead"
)
warnings.filterwarnings(
    "ignore", message="You are using `torch.load` with `weights_only=False`"
)


class SubtitleGenerator:
    def __init__(self, root):
        self.root = root
        self.root.title("Subtitle Generator")
        self.root.geometry("750x600")
        self.root.resizable(True, True)

        # Get FFmpeg path
        self.ffmpeg_path = imageio_ffmpeg.get_ffmpeg_exe()

        # Add FFmpeg to environment PATH
        ffmpeg_dir = os.path.dirname(self.ffmpeg_path)
        os.environ["PATH"] = ffmpeg_dir + os.pathsep + os.environ.get("PATH", "")

        # Apply Whisper audio loading patch
        self.patch_whisper_audio_loading()

        # Initialize variables
        self.video_path = tk.StringVar()
        self.output_folder = tk.StringVar()
        self.model_size = tk.StringVar(value="tiny")
        self.language = tk.StringVar(value="auto")
        self.status = tk.StringVar(value="Ready")
        self.progress = tk.DoubleVar(value=0)
        self.is_processing = False
        self.log_text = ""

        # Create UI
        self.create_ui()

        # Configure style
        self.style = ttk.Style()
        self.style.configure("TButton", padding=6, relief="flat", background="#4CAF50")
        self.style.configure("TLabel", padding=6)

    def create_ui(self):
        # Main frame
        main_frame = ttk.Frame(self.root, padding="10")
        main_frame.pack(fill=tk.BOTH, expand=True)

        # Input section
        input_frame = ttk.LabelFrame(main_frame, text="Input", padding="10")
        input_frame.pack(fill=tk.X, pady=5)

        ttk.Label(input_frame, text="Video File:").grid(
            row=0, column=0, sticky=tk.W, pady=5
        )
        ttk.Entry(input_frame, textvariable=self.video_path, width=50).grid(
            row=0, column=1, sticky=tk.W + tk.E, pady=5, padx=5
        )
        ttk.Button(input_frame, text="Browse", command=self.browse_video).grid(
            row=0, column=2, sticky=tk.E, pady=5
        )

        ttk.Label(input_frame, text="Output Folder:").grid(
            row=1, column=0, sticky=tk.W, pady=5
        )
        ttk.Entry(input_frame, textvariable=self.output_folder, width=50).grid(
            row=1, column=1, sticky=tk.W + tk.E, pady=5, padx=5
        )
        ttk.Button(input_frame, text="Browse", command=self.browse_output_folder).grid(
            row=1, column=2, sticky=tk.E, pady=5
        )

        # Settings section
        settings_frame = ttk.LabelFrame(main_frame, text="Settings", padding="10")
        settings_frame.pack(fill=tk.X, pady=5)

        ttk.Label(settings_frame, text="Model Size:").grid(
            row=0, column=0, sticky=tk.W, pady=5
        )
        model_combo = ttk.Combobox(
            settings_frame, textvariable=self.model_size, state="readonly"
        )
        model_combo["values"] = ("tiny", "base", "small", "medium", "large")
        model_combo.grid(row=0, column=1, sticky=tk.W, pady=5)
        ttk.Label(settings_frame, text="Smaller = Faster, Larger = More Accurate").grid(
            row=0, column=2, sticky=tk.W, pady=5, padx=10
        )

        ttk.Label(settings_frame, text="Language:").grid(
            row=1, column=0, sticky=tk.W, pady=5
        )
        language_combo = ttk.Combobox(
            settings_frame, textvariable=self.language, state="readonly"
        )
        language_combo["values"] = (
            "auto",
            "en",
            "fr",
            "de",
            "es",
            "it",
            "ja",
            "zh",
            "nl",
            "uk",
            "pt",
        )
        language_combo.grid(row=1, column=1, sticky=tk.W, pady=5)
        ttk.Label(settings_frame, text="Auto = Detect Language").grid(
            row=1, column=2, sticky=tk.W, pady=5, padx=10
        )

        # Buttons
        button_frame = ttk.Frame(main_frame)
        button_frame.pack(fill=tk.X, pady=10)

        ttk.Button(
            button_frame,
            text="Generate Subtitles",
            command=self.start_processing,
            style="TButton",
        ).pack(side=tk.LEFT, padx=5)
        ttk.Button(button_frame, text="Cancel", command=self.cancel_processing).pack(
            side=tk.LEFT, padx=5
        )
        ttk.Button(
            button_frame, text="Open Output Folder", command=self.open_output_folder
        ).pack(side=tk.LEFT, padx=5)

        # Progress section
        progress_frame = ttk.Frame(main_frame)
        progress_frame.pack(fill=tk.X, pady=5)

        ttk.Label(progress_frame, text="Status:").pack(side=tk.LEFT, padx=5)
        ttk.Label(progress_frame, textvariable=self.status).pack(side=tk.LEFT, padx=5)

        self.progress_bar = ttk.Progressbar(
            progress_frame, variable=self.progress, length=200, mode="determinate"
        )
        self.progress_bar.pack(side=tk.RIGHT, padx=5, fill=tk.X, expand=True)

        # Log section
        log_frame = ttk.LabelFrame(main_frame, text="Log", padding="10")
        log_frame.pack(fill=tk.BOTH, expand=True, pady=5)

        self.log_area = tk.Text(log_frame, wrap=tk.WORD, height=10)
        self.log_area.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

        scrollbar = ttk.Scrollbar(log_frame, command=self.log_area.yview)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        self.log_area.config(yscrollcommand=scrollbar.set)
        self.log_area.config(state=tk.DISABLED)

        # Help section
        help_frame = ttk.Frame(main_frame)
        help_frame.pack(fill=tk.X, pady=5)

        ttk.Button(help_frame, text="Help", command=self.show_help).pack(
            side=tk.LEFT, padx=5
        )
        ttk.Button(help_frame, text="About", command=self.show_about).pack(
            side=tk.LEFT, padx=5
        )

    def patch_whisper_audio_loading(self):
        from whisper.audio import load_audio

        # Create a patched version that uses our FFmpeg path
        def patched_load_audio(file, sr=16000):
            """
            Load an audio file and return a numpy array of the audio data, resampled to 16kHz.
            """
            try:
                # Use our FFmpeg path
                cmd = [
                    self.ffmpeg_path,
                    "-nostdin",
                    "-threads",
                    "0",
                    "-i",
                    file,
                    "-f",
                    "s16le",
                    "-ac",
                    "1",
                    "-acodec",
                    "pcm_s16le",
                    "-ar",
                    str(sr),
                    "-",
                ]

                self.log(f"Loading audio with FFmpeg...")
                out = subprocess.run(cmd, capture_output=True, check=True).stdout
                return (
                    np.frombuffer(out, np.int16).flatten().astype(np.float32) / 32768.0
                )
            except subprocess.CalledProcessError as e:
                error_msg = e.stderr.decode() if e.stderr else "Unknown error"
                self.log(f"FFmpeg error: {error_msg}")
                raise RuntimeError(f"FFmpeg error: {error_msg}") from e

        # Replace the original function with our patched version
        import whisper.audio

        whisper.audio.load_audio = patched_load_audio

    def browse_video(self):
        file_path = filedialog.askopenfilename(
            title="Select Video File",
            filetypes=[
                ("Video Files", "*.mp4 *.mkv *.avi *.mov *.flv *.wmv"),
                ("All Files", "*.*"),
            ],
        )
        if file_path:
            self.video_path.set(file_path)
            # Auto-set output folder to video location
            self.output_folder.set(os.path.dirname(file_path))
            self.log(f"Selected video: {file_path}")

    def browse_output_folder(self):
        folder_path = filedialog.askdirectory(title="Select Output Folder")
        if folder_path:
            self.output_folder.set(folder_path)
            self.log(f"Selected output folder: {folder_path}")

    def open_output_folder(self):
        folder = self.output_folder.get()
        if folder and os.path.exists(folder):
            if os.name == "nt":  # Windows
                os.startfile(folder)
            elif os.name == "posix":  # macOS or Linux
                subprocess.call(("open" if os.name == "posix" else "xdg-open", folder))
            self.log(f"Opening folder: {folder}")
        else:
            messagebox.showerror("Error", "Output folder doesn't exist")

    def log(self, message):
        self.log_text += f"{message}\n"
        self.log_area.config(state=tk.NORMAL)
        self.log_area.delete(1.0, tk.END)
        self.log_area.insert(tk.END, self.log_text)
        self.log_area.see(tk.END)
        self.log_area.config(state=tk.DISABLED)
        self.root.update_idletasks()

    def show_help(self):
        help_text = """
        How to use Subtitle Generator:

        1. Select a video file by clicking "Browse"
        2. Choose an output folder or use the default (same as video)
        3. Select a model size:
           - tiny: Fastest, less accurate
           - base: Good balance
           - small: Better quality but slower
           - medium: High quality but slower
           - large: Best quality but slowest
        4. Select language or leave as "auto" for automatic detection
        5. Click "Generate Subtitles" to start

        The application will:
        - Extract audio from your video
        - Transcribe the audio using Whisper AI
        - Generate SRT subtitle file

        Note: Processing large files can take several minutes.
        """
        messagebox.showinfo("Help", help_text)

    def show_about(self):
        about_text = """
        Subtitle Generator v1.0

        This application generates subtitle files (SRT) for videos
        using OpenAI's Whisper speech recognition model.

        Features:
        - Automatic speech recognition
        - Multiple language support
        - Adjustable quality settings

        Powered by:
        - Whisper (OpenAI)
        - FFmpeg
        - Python
        """
        messagebox.showinfo("About", about_text)

    def start_processing(self):
        if self.is_processing:
            messagebox.showwarning("Warning", "Already processing a video")
            return

        video_path = self.video_path.get()
        output_folder = self.output_folder.get()

        if not video_path:
            messagebox.showerror("Error", "Please select a video file")
            return

        if not os.path.exists(video_path):
            messagebox.showerror("Error", "Video file not found")
            return

        if not output_folder:
            # Default to video file's directory
            output_folder = os.path.dirname(video_path)
            self.output_folder.set(output_folder)

        if not os.path.exists(output_folder):
            try:
                os.makedirs(output_folder)
            except Exception as e:
                messagebox.showerror("Error", f"Couldn't create output folder: {e}")
                return

        # Start processing in a separate thread
        self.is_processing = True
        threading.Thread(target=self.process_video, daemon=True).start()

    def process_video(self):
        try:
            video_path = self.video_path.get()
            output_folder = self.output_folder.get()
            model_size = self.model_size.get()
            language = self.language.get() if self.language.get() != "auto" else None

            self.status.set("Extracting audio...")
            self.progress.set(10)

            # Extract audio
            audio_path = self.extract_audio(video_path)
            if not audio_path:
                self.status.set("Failed to extract audio")
                self.is_processing = False
                return

            self.status.set("Loading Whisper model...")
            self.progress.set(30)
            self.log(f"Loading {model_size} model...")

            # Load model
            model = whisper.load_model(model_size)

            self.status.set("Transcribing audio...")
            self.progress.set(50)
            self.log("Transcribing audio... (this may take a while)")

            # Transcribe
            transcribe_options = {"language": language} if language else {}
            result = model.transcribe(audio_path, **transcribe_options)

            self.status.set("Generating subtitles...")
            self.progress.set(80)

            # Generate SRT
            segments = result["segments"]
            subtitles = self.generate_srt(segments)

            # Save to output file
            output_filename = os.path.splitext(os.path.basename(video_path))[0] + ".srt"
            output_path = os.path.join(output_folder, output_filename)

            with open(output_path, "w", encoding="utf-8") as f:
                f.write(subtitles)

            self.status.set("Completed")
            self.progress.set(100)
            self.log(f"Subtitles saved to: {output_path}")

            messagebox.showinfo(
                "Success",
                f"Subtitles generated successfully!\n\nSaved to: {output_path}",
            )

        except Exception as e:
            self.status.set("Error")
            self.log(f"Error: {str(e)}")
            messagebox.showerror("Error", f"An error occurred: {str(e)}")

        finally:
            self.is_processing = False

    def extract_audio(self, video_path):
        try:
            # Generate the output audio file path
            audio_path = os.path.join(
                self.output_folder.get(),
                os.path.splitext(os.path.basename(video_path))[0] + ".mp3",
            )

            if os.path.exists(audio_path) and os.path.isfile(audio_path):
                self.log(f"Audio file is already present")
                return audio_path

            # Properly quote the file paths for cmd
            command = [
                self.ffmpeg_path,
                "-i",
                video_path,
                "-vn",  # No video
                "-acodec",
                "mp3",
                "-ar",
                "44100",
                "-ac",
                "2",
                "-ab",
                "192k",
                audio_path,
            ]

            self.log(f"Extracting audio...")
            subprocess.run(command, check=True, capture_output=True)
            self.log(f"Audio extracted to: {audio_path}")
            return audio_path

        except Exception as e:
            self.log(f"Error extracting audio: {e}")
            return None

    def generate_srt(self, segments):
        subtitles = []
        for index, segment in enumerate(segments):
            start = datetime.timedelta(seconds=segment["start"])
            end = datetime.timedelta(seconds=segment["end"])
            content = segment["text"]
            subtitle = srt.Subtitle(
                index=index + 1, start=start, end=end, content=content
            )
            subtitles.append(subtitle)
        return srt.compose(subtitles)

    def cancel_processing(self):
        if not self.is_processing:
            return

        if messagebox.askyesno("Cancel", "Cancel the current process?"):
            self.log("Cancelling process... (This may take a moment)")
            self.status.set("Cancelling...")
            self.is_processing = False


# Run the application
if __name__ == "__main__":
    root = tk.Tk()
    app = SubtitleGenerator(root)
    root.mainloop()
