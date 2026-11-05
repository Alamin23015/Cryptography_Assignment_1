import subprocess
import sys
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


class SimulationIntegrationTests(unittest.TestCase):
    def test_visualizer_runs_seven_layer_socket_round_trip(self):
        result = subprocess.run(
            [sys.executable, str(ROOT / "python" / "visualizer.py"),
             "--message", "OSI integration test"],
            cwd=ROOT,
            capture_output=True,
            text=True,
            timeout=20,
            check=False,
        )

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("Client encapsulation (sender)", result.stdout)
        self.assertIn("Server decapsulation (receiver)", result.stdout)
        self.assertIn("Server encapsulation (acknowledgement)", result.stdout)
        self.assertIn("Client decapsulation (acknowledgement)", result.stdout)
        self.assertIn("Progress: layers 7-6 complete", result.stdout)
        self.assertIn("Progress: layers 1-2 complete", result.stdout)
        self.assertIn("Delivered message: ACK: OSI integration test", result.stdout)


if __name__ == "__main__":
    unittest.main()