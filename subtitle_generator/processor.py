"""
Processor Module

Contains functionality for processing video and audio files,
including audio extraction, transcription, and subtitle generation.
"""

import os
import subprocess
import datetime
import srt
import whisper


class SubtitleProcessor:
    """Handles the processing of videos to generate subtitles."""

    def __init__(self, ffmpeg_path, log_callback=None):
        """
        Initialize the SubtitleProcessor.

        Args:
            ffmpeg_path (str): Path to the FFmpeg executable
            log_callback (callable, optional): Function to call for logging
        """
        self.ffmpeg_path = ffmpeg_path
        self.log_callback = log_callback

    def log(self, message):
        """Log a message using the callback if provided."""
        if self.log_callback:
            self.log_callback(message)

    def extract_audio(self, video_path, output_folder):
        """
        Extract audio from a video file.

        Args:
            video_path (str): Path to the video file
            output_folder (str): Path to the folder where the audio file should be saved

        Returns:
            str: Path to the extracted audio file or None if extraction failed

        Raises:
            Exception: If audio extraction fails
        """
        try:
            # Generate the output audio file path
            audio_path = os.path.join(
                output_folder,
                os.path.splitext(os.path.basename(video_path))[0] + ".mp3",
            )

            if os.path.exists(audio_path) and os.path.isfile(audio_path):
                self.log(f"Audio file is already present")
                return audio_path

            # Setup the FFmpeg command
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
            raise

    def transcribe_audio(self, audio_path, model_size="base", language=None):
        """
        Transcribe audio using the Whisper model.

        Args:
            audio_path (str): Path to the audio file
            model_size (str): Size of the Whisper model to use
            language (str, optional): Language code or None for auto-detection

        Returns:
            dict: Transcription result

        Raises:
            Exception: If transcription fails
        """
        try:
            self.log(f"Loading {model_size} model...")
            model = whisper.load_model(model_size)

            self.log("Transcribing audio... (this may take a while)")
            transcribe_options = {"language": language} if language else {}
            result = model.transcribe(audio_path, **transcribe_options)

            return result
        except Exception as e:
            self.log(f"Error transcribing audio: {e}")
            raise

    def generate_srt(self, segments):
        """
        Generate SRT subtitles from transcription segments.

        Args:
            segments (list): List of segment dictionaries from the Whisper result

        Returns:
            str: Composed SRT content
        """
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

    def save_subtitles(self, subtitles, video_path, output_folder):
        """
        Save subtitles to an SRT file.

        Args:
            subtitles (str): SRT content to save
            video_path (str): Path to the original video file (used to generate the SRT filename)
            output_folder (str): Folder where to save the SRT file

        Returns:
            str: Path to the saved SRT file
        """
        output_filename = os.path.splitext(os.path.basename(video_path))[0] + ".srt"
        output_path = os.path.join(output_folder, output_filename)

        with open(output_path, "w", encoding="utf-8") as f:
            f.write(subtitles)

        self.log(f"Subtitles saved to: {output_path}")
        return output_path

    def process_video(
        self, video_path, output_folder, model_size="base", language=None
    ):
        """
        Process a video file to generate subtitles.

        Args:
            video_path (str): Path to the video file
            output_folder (str): Path to the output folder
            model_size (str): Whisper model size to use
            language (str, optional): Language code or None for auto-detection

        Returns:
            str: Path to the generated SRT file

        Raises:
            Exception: If any step of the process fails
        """
        # Extract audio
        audio_path = self.extract_audio(video_path, output_folder)

        # Transcribe audio
        result = self.transcribe_audio(audio_path, model_size, language)

        # Generate SRT
        self.log("Generating subtitles...")
        subtitles = self.generate_srt(result["segments"])

        # Save to file
        return self.save_subtitles(subtitles, video_path, output_folder)
