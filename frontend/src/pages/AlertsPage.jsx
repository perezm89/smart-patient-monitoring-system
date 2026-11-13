
import { useEffect, useState } from 'react';

import Sidebar from '../components/Sidebar';
import api from '../services/api';

// Refresh alerts every 5 seconds.
const POLLING_INTERVAL_MS = 5000;

function AlertsPage() {
  const [alerts, setAlerts] = useState([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState('');
  const [acknowledgingId, setAcknowledgingId] = useState(null);

  // Only caregivers can acknowledge alerts.
  // The backend independently enforces this restriction.
  const isCaregiver =
    sessionStorage.getItem('userRole') === 'caregiver';

  // ACKNOWLEDGE ALERT
  const handleAcknowledge = async (alertId) => {
    if (!isCaregiver || acknowledgingId !== null) {
      return;
    }

    setAcknowledgingId(alertId);

    try {
      const response = await api.patch(
        `/api/v1/alerts/${alertId}/acknowledge`
      );

      const updatedAlert = response.data.data;

      // Update the displayed alert using the
      // confirmed response from the backend.
      setAlerts((previousAlerts) =>
        previousAlerts.map((alert) =>
          alert._id === alertId
            ? updatedAlert
            : alert
        )
      );

      setError('');
    } catch (err) {
      console.error(
        'Failed to acknowledge alert:',
        err
      );

      setError('Unable to acknowledge alert');
    } finally {
      setAcknowledgingId(null);
    }
  };

  // FETCH ALERTS + AUTOMATIC POLLING
  useEffect(() => {
    let isMounted = true;
    let isFetching = false;

    const fetchAlerts = async () => {
      if (isFetching) return;

      isFetching = true;

      try {
        // JWT is automatically attached by api.
        const response = await api.get(
          '/api/v1/alerts'
        );

        const retrievedAlerts = response.data.data;

        if (!Array.isArray(retrievedAlerts)) {
          throw new Error('Invalid alerts response');
        }

        if (!isMounted) return;

        setAlerts(retrievedAlerts);
        setError('');
      } catch (err) {
        console.error(
          'Failed to retrieve alerts:',
          err
        );

        if (isMounted) {
          setError('Unable to retrieve alerts');
        }
      } finally {
        isFetching = false;

        if (isMounted) {
          setLoading(false);
        }
      }
    };

    // Initial request.
    fetchAlerts();

    // Refresh alerts every 5 seconds.
    const intervalId = setInterval(
      fetchAlerts,
      POLLING_INTERVAL_MS
    );

    // Cleanup when leaving the page.
    return () => {
      isMounted = false;
      clearInterval(intervalId);
    };
  }, []);

  // ALERTS PAGE UI
  return (
    <div className="dashboard-layout">
      <Sidebar />

      <main className="dashboard-main">
        <header className="dashboard-header">
          <h1>Alerts</h1>

          <p>
            Fall detection events and acknowledgment
            status
          </p>
        </header>

        {/* Loading and error messages */}
        {loading && (
          <p>Loading alerts...</p>
        )}

        {error && (
          <p role="alert">{error}</p>
        )}

        {/* Empty state */}
        {!loading && !error && alerts.length === 0 && (
          <p>No alerts recorded.</p>
        )}

        {/* Alert cards */}
        <section className="alerts-list">
          {alerts.map((alert) => (
            <article
              key={alert._id}
              className="alert-item"
            >
              <h3>
                {alert.type === 'fall'
                  ? 'Fall Detected'
                  : alert.type}
              </h3>

              <p>
                Device: {alert.deviceId}
              </p>

              <p>
                Recorded:{' '}
                {new Date(
                  alert.timestamp
                ).toLocaleString()}
              </p>

              <p>
                Status:{' '}
                <span
                  className={
                    alert.status === 'acknowledged'
                      ? 'alert-status-acknowledged'
                      : 'alert-status-active'
                  }
                >
                  {alert.status === 'acknowledged'
                    ? 'Acknowledged'
                    : 'Active'}
                </span>
              </p>

              {/* Only caregivers can acknowledge
                  active alerts. */}
              {isCaregiver &&
                alert.status === 'active' && (
                  <button
                    type="button"
                    className="alert-acknowledge-button"
                    onClick={() =>
                      handleAcknowledge(alert._id)
                    }
                    disabled={acknowledgingId !== null}
                  >
                    {acknowledgingId === alert._id
                      ? 'Acknowledging...'
                      : 'Acknowledge'}
                  </button>
                )}
            </article>
          ))}
        </section>
      </main>
    </div>
  );
}

export default AlertsPage;
