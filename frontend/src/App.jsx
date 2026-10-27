import {
  BrowserRouter,
  Routes,
  Route,
  Navigate
} from 'react-router-dom';

import Sidebar from './components/Sidebar';
import VitalCard from './components/VitalCard';
import VitalsChart from './components/VitalsChart';
import LoginPage from './pages/LoginPage';

import './App.css';

function Dashboard() {
  return (
    <div className="dashboard-layout">
      <Sidebar />

      <main className="dashboard-main">
        <header className="dashboard-header">
          <h1>Patient Dashboard</h1>
          <p>Real-time patient vitals and device status</p>
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

        <VitalsChart />
      </main>
    </div>
  );
}

function ProtectedRoute({ children }) {
  const token = sessionStorage.getItem('accessToken');

  if (!token) {
    return <Navigate to="/login" replace />;
  }

  return children;
}

function App() {
  return (
    <BrowserRouter>
      <Routes>
        <Route path="/" element={<Navigate to="/dashboard" replace />} />

        <Route path="/login" element={<LoginPage />} />

        <Route
          path="/dashboard"
          element={
            <ProtectedRoute>
              <Dashboard />
            </ProtectedRoute>
          }
        />

        <Route
          path="/vitals"
          element={<Navigate to="/dashboard" replace />}
        />
      </Routes>
    </BrowserRouter>
  );
}

export default App;