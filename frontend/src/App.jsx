
import {
  BrowserRouter,
  Routes,
  Route,
  Navigate
} from 'react-router-dom';

import { useEffect, useState } from 'react';

// Axios client for communicating with our Express backend.
// Automatically attaches the JWT from sessionStorage.
import api from './services/api';

// Dashboard components
import Sidebar from './components/Sidebar';
import VitalCard from './components/VitalCard';
import VitalsChart from './components/VitalsChart';
import CaregiverDashboard from './components/CaregiverDashboard';

// Authentication pages
import LoginPage from './pages/LoginPage';
import SignupPage from './pages/SignupPage';
import AlertsPage from './pages/AlertsPage';

import './App.css';

// Polling interval: request updated measurements every 5 seconds.
const POLLING_INTERVAL_MS = 5000;

// PATIENT DASHBOARD
function Dashboard() {

  // Most recent measurement for the vital-sign cards.
  const [latestVitals, setLatestVitals] = useState(null);

  // Historical measurements for the Recharts graph.
  const [vitalsHistory, setVitalsHistory] = useState([]);

  // Track the initial API request and any errors.
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState('');


  // FETCH PATIENT VITALS + AUTOMATIC POLLING
  useEffect(() => {

    // Track whether this effect is still active.
    // Prevents state updates after unmounting.
    let isMounted = true;

    // Prevent overlapping requests if the backend is slow.
    let isFetching = false;

    const fetchVitals = async () => {

      // Skip if a previous request is still running.
      if (isFetching) return;

      isFetching = true;

      try {

        // GET request to our protected Express endpoint.
        // The Axios interceptor automatically includes the JWT.
        const response = await api.get(
          '/api/v1/patients/me/vitals'
        );

        // Expected response:
        // {
        //   success: true,
        //   data: [ ...vital records ]
        // }
        const vitals = response.data.data;

        // Ignore results if this effect has been cleaned up.
        if (!isMounted) return;

        // Validate that the backend returned an array.
        if (!Array.isArray(vitals)) {
          throw new Error('Invalid vitals response');
        }

        if (vitals.length > 0) {

          // Sort records chronologically, oldest to newest.
          // Copy the array to avoid modifying the response.
          const sortedVitals = [...vitals].sort(
            (a, b) =>
              new Date(a.timestamp) - new Date(b.timestamp)
          );

          // Update the historical chart.
          setVitalsHistory(sortedVitals);

          // Update cards with the newest measurement.
          setLatestVitals(
            sortedVitals[sortedVitals.length - 1]
          );

        } else {

          // No measurements stored for this patient.
          setVitalsHistory([]);
          setLatestVitals(null);
        }

        // Clear any previous API errors.
        setError('');

      } catch (err) {

        console.error('Failed to fetch vitals:', err);

        if (isMounted) {
          setError('Unable to refresh patient vitals');
        }

      } finally {

        // Allow the next polling request.
        isFetching = false;

        if (isMounted) {
          setLoading(false);
        }
      }
    };


    // Request measurements immediately on dashboard load.
    fetchVitals();

    // Repeat the request every 5 seconds.
    // New MongoDB records will appear automatically.
    const intervalId = setInterval(
      fetchVitals,
      POLLING_INTERVAL_MS
    );


    // Cleanup when the dashboard unmounts.
    // Also handles React Strict Mode effect cleanup.
    return () => {
      isMounted = false;
      clearInterval(intervalId);
    };

  }, []);


  // PATIENT DASHBOARD UI
  return (
    <div className="dashboard-layout">

      {/* Left navigation sidebar */}
      <Sidebar />

      <main className="dashboard-main">

        {/* Dashboard heading and measurement timestamp */}
        <header className="dashboard-header">

          <h1>Patient Dashboard</h1>

          <p>Recorded patient vitals and device status</p>

          {/* Timestamp of the latest stored measurement.
              This is NOT the time of the latest API request. */}
          {latestVitals?.timestamp && (
            <p>
              Last measurement:{' '}
              {new Date(
                latestVitals.timestamp
              ).toLocaleString()}
            </p>
          )}

        </header>


        {/* API loading and error messages */}
        {loading && (
          <p>Loading patient vitals...</p>
        )}

        {error && (
          <p role="alert">{error}</p>
        )}


        {/* VITAL-SIGN SUMMARY CARDS */}

        <section className="vitals-grid">

          {/* Heart Rate */}
          <VitalCard
            title="Heart Rate"
            value={latestVitals?.heartRateBpm ?? '--'}
            unit="BPM"
            icon="❤️"
            status={
              latestVitals?.heartRateBpm != null
                ? 'Recorded reading available'
                : 'Awaiting sensor data'
            }
          />


          {/* Blood Oxygen Saturation */}
          <VitalCard
            title="SpO₂"
            value={latestVitals?.spo2Percent ?? '--'}
            unit="%"
            icon="🫁"
            status={
              latestVitals?.spo2Percent != null
                ? 'Recorded reading available'
                : 'Awaiting sensor data'
            }
          />


          {/* Skin Temperature */}
          <VitalCard
            title="Skin Temperature"
            value={
              latestVitals?.skinTemperatureC != null
                ? Number(
                    latestVitals.skinTemperatureC
                  ).toFixed(2)
                : '--'
            }
            unit="°C"
            icon="🌡️"
            status={
              latestVitals?.skinTemperatureC != null
                ? 'Recorded reading available'
                : 'Awaiting sensor data'
            }
          />


          
          {/* Display fall detection from the latest patient reading */}
          <VitalCard
            title="Fall Detection"
            value={
              latestVitals?.fallDetected === true
                ? "Fall Detected"
                : latestVitals?.fallDetected === false
                  ? "No Fall"
                  : "--"
            }
            unit=""
            icon="🏃"
            status={
              latestVitals?.fallDetected === true
                ? "Fall event recorded - attention required"
                : latestVitals?.fallDetected === false
                  ? "No fall in latest reading"
                  : "Awaiting sensor data"
            }
            variant={
              latestVitals?.fallDetected === true
                ? "danger"
                : "normal"
            }
          />


        </section>


        {/* HISTORICAL VITAL-SIGN CHART */}

        {/* Pass the measurements retrieved from MongoDB.
            The chart automatically re-renders whenever
            polling updates vitalsHistory. */}
        <VitalsChart vitals={vitalsHistory} />

      </main>
    </div>
  );
}

// ROLE-PROTECTED ROUTE
// Checks whether the user has a JWT and the required role.
// These checks control frontend navigation only.
// The Express backend independently validates JWTs and
// enforces authorization before returning patient data.

function ProtectedRoute({ children, requiredRole }) {

  const token = sessionStorage.getItem('accessToken');
  const userRole = sessionStorage.getItem('userRole');

  // No token: redirect to login.
  if (!token) {
    return <Navigate to="/login" replace />;
  }

  // Missing or unrecognized role: require a fresh login.
  if (!['patient', 'caregiver'].includes(userRole)) {
    return <Navigate to="/login" replace />;
  }

  // Wrong role: redirect to the appropriate dashboard.
  if (requiredRole && userRole !== requiredRole) {

    const correctDashboard =
      userRole === 'caregiver'
        ? '/caregiver/dashboard'
        : '/dashboard';

    return (
      <Navigate
        to={correctDashboard}
        replace
      />
    );
  }

  return children;
}

// APPLICATION ROUTING
function App() {

  // Read the role stored during login.
  const userRole = sessionStorage.getItem('userRole');

  // Determine the appropriate dashboard for the user.
  const defaultDashboard =
    userRole === 'caregiver'
      ? '/caregiver/dashboard'
      : '/dashboard';

  return (
    <BrowserRouter>

      <Routes>

        {/* Default route */}
        <Route
          path="/"
          element={
            <Navigate to={defaultDashboard} replace />
          }
        />


        {/* Public login page */}
        <Route
          path="/login"
          element={<LoginPage />}
        />


        {/* Public signup page */}
        <Route
          path="/signup"
          element={<SignupPage />}
        />


        {/* Protected patient dashboard */}
        <Route
          path="/dashboard"
          element={
            <ProtectedRoute requiredRole="patient">
              <Dashboard />
            </ProtectedRoute>
          }
        />


        {/* Protected caregiver dashboard */}
        <Route
          path="/caregiver/dashboard"
          element={
            <ProtectedRoute requiredRole="caregiver">
              <CaregiverDashboard />
            </ProtectedRoute>
          }
        />
        
        {/* Protected alerts page for patients and caregivers */}
        <Route
          path="/alerts"
          element={
            <ProtectedRoute>
              <AlertsPage />
            </ProtectedRoute>
          }
        />

        {/*Redirect to the appropriate dashboard.
          Both patient and caregiver dashboards already
          display vital measurements and historical charts. */}
        <Route
          path="/vitals"
          element={
            <Navigate to={defaultDashboard} replace />
          }
        />

      </Routes>

    </BrowserRouter>
  );
}

export default App;
