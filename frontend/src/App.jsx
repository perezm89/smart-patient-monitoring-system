import { BrowserRouter } from 'react-router-dom';
import Sidebar from './components/Sidebar';
import VitalCard from './components/VitalCard';
import './App.css';

function App() {
  return (
    <BrowserRouter>
      <div className="dashboard-layout">
        <Sidebar />

        <main className="dashboard-main">
          <header className="dashboard-header">
            <div>
              <h1>Patient Dashboard</h1>
              <p>Real-time patient vitals and device status</p>
            </div>
          </header>
          <section className="vitals-grid">
            <VitalCard
              title="Heart Rate"
              value="--"
              unit="BPM"
              icon="❤️"
              status="Awaiting sensor data"
            />

            <VitalCard
              title="SpO₂"
              value="--"
              unit="%"
              icon="🫁"
              status="Awaiting sensor data"
            />

            <VitalCard
              title="Skin Temperature"
              value="--"
              unit="°C"
              icon="🌡️"
              status="Awaiting sensor data"
            />

            <VitalCard
              title="Fall Detection"
              value="--"
              unit=""
              icon="🏃"
              status="Not connected"
            />
          </section>
          
        </main>
      </div>
    </BrowserRouter>
  );
}

export default App;