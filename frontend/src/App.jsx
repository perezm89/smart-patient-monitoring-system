
import {
  BrowserRouter,
  Routes,
  Route,
  Navigate
} from 'react-router-dom';

import { useEffect, useState } from 'react';
import api from './services/api';

import Sidebar from './components/Sidebar';
import VitalCard from './components/VitalCard';
import VitalsChart from './components/VitalsChart';
import LoginPage from './pages/LoginPage';

import './App.css';

function Dashboard() {
  const [latestVitals, setLatestVitals] = useState(null);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState('');

  useEffect(() => {
    const fetchVitals = async () => {
      try {
        const response = await api.get('/api/v1/patients/me/vitals');

        const vitals = response.data.data;

        if (Array.isArray(vitals) && vitals.length > 0) {
          setLatestVitals(vitals[0]);
        } else {
          setLatestVitals(null);
        }

        setError('');
      } catch (err) {
        console.error('Failed to fetch vitals:', err);
        setError('Unable to load patient vitals');
      } finally {
        setLoading(false);
      }
    };

    fetchVitals();
  }, []);

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
            value={latestVitals?.heartRateBpm ?? '--'}
            unit="BPM"
            icon="❤️"
            status="Awaiting sensor data"
          />

          <VitalCard
            title="SpO₂"
            value={latestVitals?.spo2Percent ?? '--'}
            unit="%"
            icon="🫁"
            status="Awaiting sensor data"
          />

          <VitalCard
            title="Skin Temperature"
            value={latestVitals?.skinTemperatureC?.toFixed(2) ?? '--'}
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
        <Route
          path="/"
          element={<Navigate to="/dashboard" replace />}
        />

        <Route
          path="/login"
          element={<LoginPage />}
        />

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
