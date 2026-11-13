
import { useEffect, useState } from 'react';

import Sidebar from '../components/Sidebar';
import api from '../services/api';

// Refresh device information every 5 seconds.
const POLLING_INTERVAL_MS = 5000;

function DevicesPage() {
  const [devices, setDevices] = useState([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState('');

  // Format MongoDB timestamps for display.
  const formatTimestamp = (timestamp) => {
    if (!timestamp) {
      return 'No readings yet';
    }

    const date = new Date(timestamp);

    if (Number.isNaN(date.getTime())) {
      return 'Unavailable';
    }

    return date.toLocaleString();
  };

  // FETCH DEVICES + AUTOMATIC POLLING
  useEffect(() => {
    let isMounted = true;
    let isFetching = false;

    const fetchDevices = async () => {
      if (isFetching) return;

      isFetching = true;

      try {
        // JWT is automatically attached by api.
        const response = await api.get(
          '/api/v1/devices'
        );

        const retrievedDevices = response.data.data;

        if (!Array.isArray(retrievedDevices)) {
          throw new Error('Invalid devices response');
        }

        if (!isMounted) return;

        setDevices(retrievedDevices);
        setError('');
      } catch (err) {
        console.error(
          'Failed to retrieve devices:',
          err
        );

        if (isMounted) {
          setError('Unable to retrieve devices');
        }
      } finally {
        isFetching = false;

        if (isMounted) {
          setLoading(false);
        }
      }
    };

    // Initial request.
    fetchDevices();

    // Refresh every 5 seconds.
    const intervalId = setInterval(
      fetchDevices,
      POLLING_INTERVAL_MS
    );

    // Cleanup when leaving the page.
    return () => {
      isMounted = false;
      clearInterval(intervalId);
    };
  }, []);

  return (
    <div className="dashboard-layout">
      <Sidebar />

      <main className="dashboard-main">
        <header className="dashboard-header">
          <h1>Devices</h1>

          <p>
            Registered patient monitoring devices
          </p>
        </header>

        {loading && (
          <p>Loading devices...</p>
        )}

        {error && (
          <p role="alert">{error}</p>
        )}

        {!loading &&
          !error &&
          devices.length === 0 && (
            <p>No devices assigned.</p>
          )}

        <section className="devices-list">
          {devices.map((device) => {
            const patient = device.assignedPatient;

            const patientName = patient
              ? [
                  patient.firstName,
                  patient.lastName
                ]
                  .filter(Boolean)
                  .join(' ') || 'Unknown patient'
              : 'Unassigned';

            return (
              <article
                key={device.id}
                className="device-item"
              >
                <div className="device-item-header">
                  <h3>{device.deviceId}</h3>

                  <span
                    className={
                      device.active
                        ? 'device-status-active'
                        : 'device-status-inactive'
                    }
                  >
                    {device.active
                      ? 'Enabled'
                      : 'Disabled'}
                  </span>
                </div>

                <p>
                  <strong>Assigned Patient:</strong>{' '}
                  {patientName}
                </p>

                <p>
                  <strong>Registered:</strong>{' '}
                  {device.registeredAt
                    ? formatTimestamp(device.registeredAt)
                    : 'Unavailable'}
                </p>

                <p>
                  <strong>Last Measurement:</strong>{' '}
                  {formatTimestamp(device.lastReadingAt)}
                </p>
              </article>
            );
          })}
        </section>
      </main>
    </div>
  );
}

export default DevicesPage;
