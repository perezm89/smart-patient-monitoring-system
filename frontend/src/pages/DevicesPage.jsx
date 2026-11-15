
import { useEffect, useState } from 'react';

import Sidebar from '../components/Sidebar';
import api from '../services/api';

// Refresh device information every 5 seconds.
const POLLING_INTERVAL_MS = 5000;

function DevicesPage() {
  const [devices, setDevices] = useState([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState('');

  // Device claiming is available to patients only.
  const isPatient =
    sessionStorage.getItem('userRole') === 'patient';

  // Claim Device form state.
  const [claimDeviceId, setClaimDeviceId] = useState('');
  const [claimApiKey, setClaimApiKey] = useState('');
  const [claimLoading, setClaimLoading] = useState(false);
  const [claimError, setClaimError] = useState('');
  const [claimSuccess, setClaimSuccess] = useState('');

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

  // Claim an available device for the logged-in patient.
  const handleClaimDevice = async (event) => {
    event.preventDefault();

    if (!isPatient || claimLoading) return;

    setClaimError('');
    setClaimSuccess('');
    setClaimLoading(true);

    try {
      const response = await api.post(
        '/api/v1/devices/claim',
        {
          deviceId: claimDeviceId.trim(),
          apiKey: claimApiKey.trim()
        }
      );

      if (!response.data.success) {
        throw new Error('Device claim failed');
      }

      setClaimSuccess('Device claimed successfully!');
      setClaimDeviceId('');

      // Refresh the list immediately after claiming.
      // A refresh failure does not undo a successful claim.
      try {
        const devicesResponse = await api.get(
          '/api/v1/devices'
        );

        if (Array.isArray(devicesResponse.data.data)) {
          setDevices(devicesResponse.data.data);
          setError('');
        }
      } catch (refreshError) {
        console.error(
          'Unable to refresh devices after claiming:',
          refreshError
        );
        // Automatic polling will retry.
      }

    } catch (err) {
      setClaimError(
        err.response?.data?.message ||
        err.message ||
        'Unable to claim device'
      );
    } finally {
      // Do not retain the claim credential after submission.
      setClaimApiKey('');
      setClaimLoading(false);
    }
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

        {/* PATIENT-ONLY DEVICE CLAIMING */}
        {isPatient && (
          <section className="device-claim-section">
            <h2>Claim a Device</h2>

            <p>
              Enter your Device ID and claim credential
              to connect a monitoring device to your account.
            </p>

            <form onSubmit={handleClaimDevice}>
              <div>
                <label htmlFor="claim-device-id">
                  Device ID
                </label>

                <input
                  id="claim-device-id"
                  type="text"
                  value={claimDeviceId}
                  onChange={(event) =>
                    setClaimDeviceId(event.target.value)
                  }
                  placeholder="e.g. SIM-ESP32-001"
                  disabled={claimLoading}
                  required
                />
              </div>

              <div>
                <label htmlFor="claim-api-key">
                  Claim Credential
                </label>

                <input
                  id="claim-api-key"
                  type="password"
                  value={claimApiKey}
                  onChange={(event) =>
                    setClaimApiKey(event.target.value)
                  }
                  autoComplete="off"
                  disabled={claimLoading}
                  required
                />
              </div>

              <button
                type="submit"
                disabled={claimLoading}
              >
                {claimLoading
                  ? 'Claiming...'
                  : 'Claim Device'}
              </button>
            </form>

            {claimError && (
              <p role="alert">{claimError}</p>
            )}

            {claimSuccess && (
              <p role="status">{claimSuccess}</p>
            )}
          </section>
        )}

        {/* DEVICE LIST */}
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
                  <strong>Device Type:</strong>{' '}
                  {device.deviceType === 'simulated'
                    ? 'Simulated'
                    : 'Physical'}
                </p>

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
