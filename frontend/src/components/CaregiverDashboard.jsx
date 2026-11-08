
import { useEffect, useState } from 'react';

import api from '../services/api';
import Sidebar from './Sidebar';
import VitalCard from './VitalCard';
import VitalsChart from './VitalsChart';

// Refresh selected patient's measurements every 5 seconds.
const POLLING_INTERVAL_MS = 5000;

function CaregiverDashboard() {

  // Patients assigned to the authenticated caregiver.
  const [patients, setPatients] = useState([]);

  // MongoDB ID of the currently selected patient.
  const [selectedPatientId, setSelectedPatientId] = useState('');

  // Vital measurements for the selected patient.
  const [latestVitals, setLatestVitals] = useState(null);
  const [vitalsHistory, setVitalsHistory] = useState([]);

  // Loading and error states.
  const [patientsLoading, setPatientsLoading] = useState(true);
  const [vitalsLoading, setVitalsLoading] = useState(false);
  const [patientsError, setPatientsError] = useState('');
  const [vitalsError, setVitalsError] = useState('');


  // ==========================================================
  // FETCH ASSIGNED PATIENTS
  // ==========================================================

  useEffect(() => {

    let isMounted = true;

    const fetchPatients = async () => {

      try {
        const response = await api.get(
          '/api/v1/caregivers/me/patients'
        );

        const assignedPatients = response.data.data;

        if (!isMounted) return;

        if (!Array.isArray(assignedPatients)) {
          throw new Error('Invalid patients response');
        }

        setPatients(assignedPatients);

        // Automatically select the first assigned patient.
        if (assignedPatients.length > 0) {
          setSelectedPatientId(assignedPatients[0]._id);
        }

        setPatientsError('');

      } catch (err) {

        console.error('Failed to fetch assigned patients:', err);

        if (isMounted) {
          setPatientsError('Unable to load assigned patients');
        }

      } finally {

        if (isMounted) {
          setPatientsLoading(false);
        }
      }
    };

    fetchPatients();

    return () => {
      isMounted = false;
    };

  }, []);


  // ==========================================================
  // FETCH SELECTED PATIENT VITALS + POLLING
  // ==========================================================

  useEffect(() => {

    // Don't request vitals until a patient is selected.
    if (!selectedPatientId) return;

    let isMounted = true;
    let isFetching = false;

    // Clear previous patient's readings immediately.
    setLatestVitals(null);
    setVitalsHistory([]);
    setVitalsLoading(true);
    setVitalsError('');

    const fetchVitals = async () => {

      if (isFetching) return;

      isFetching = true;

      try {

        const response = await api.get(
          `/api/v1/caregivers/me/patients/${selectedPatientId}/vitals`
        );

        const vitals = response.data.data;

        if (!isMounted) return;

        if (!Array.isArray(vitals)) {
          throw new Error('Invalid vitals response');
        }

        if (vitals.length > 0) {

          // Sort oldest to newest for the chart.
          const sortedVitals = [...vitals].sort(
            (a, b) =>
              new Date(a.timestamp) - new Date(b.timestamp)
          );

          setVitalsHistory(sortedVitals);

          // Most recent reading for summary cards.
          setLatestVitals(
            sortedVitals[sortedVitals.length - 1]
          );

        } else {

          setVitalsHistory([]);
          setLatestVitals(null);
        }

        setVitalsError('');

      } catch (err) {

        console.error('Failed to fetch caregiver vitals:', err);

        if (isMounted) {
          setVitalsError('Unable to refresh patient vitals');
        }

      } finally {

        isFetching = false;

        if (isMounted) {
          setVitalsLoading(false);
        }
      }
    };

    // Fetch immediately when the selected patient changes.
    fetchVitals();

    // Refresh every five seconds.
    const intervalId = setInterval(
      fetchVitals,
      POLLING_INTERVAL_MS
    );

    // Stop polling when changing patients or leaving page.
    return () => {
      isMounted = false;
      clearInterval(intervalId);
    };

  }, [selectedPatientId]);


  // ==========================================================
  // SELECTED PATIENT
  // ==========================================================

  const selectedPatient = patients.find(
    (patient) => patient._id === selectedPatientId
  );


  // ==========================================================
  // CAREGIVER DASHBOARD UI
  // ==========================================================

  return (
    <div className="dashboard-layout">

      <Sidebar />

      <main className="dashboard-main">

        <header className="dashboard-header">
          <h1>Caregiver Dashboard</h1>
          <p>Monitor your assigned patients</p>
        </header>


        {/* ASSIGNED PATIENTS */}

        <section className="caregiver-patients">

          <h2>Assigned Patients</h2>

          {patientsLoading && (
            <p>Loading assigned patients...</p>
          )}

          {patientsError && (
            <p role="alert">{patientsError}</p>
          )}

          {!patientsLoading &&
            !patientsError &&
            patients.length === 0 && (
              <p>No patients have been assigned to you.</p>
            )}

          {patients.length > 0 && (
            <div className="patient-list">

              {patients.map((patient) => (

                <button
                  key={patient._id}
                  type="button"
                  className={
                    selectedPatientId === patient._id
                      ? 'patient-item selected'
                      : 'patient-item'
                  }
                  aria-pressed={
                    selectedPatientId === patient._id
                  }
                  onClick={() =>
                    setSelectedPatientId(patient._id)
                  }
                >

                  <strong>
                    {patient.firstName} {patient.lastName}
                  </strong>

                  <span>
                    Patient ID: {patient._id}
                  </span>

                </button>

              ))}

            </div>
          )}

        </section>


        {/* SELECTED PATIENT'S MEASUREMENTS */}

        {selectedPatient && (
          <>

            <header className="dashboard-header">

              <h2>
                {selectedPatient.firstName}{' '}
                {selectedPatient.lastName}
              </h2>

              <p>
                Patient ID: {selectedPatient._id}
              </p>

              {latestVitals?.timestamp && (
                <p>
                  Last measurement:{' '}
                  {new Date(
                    latestVitals.timestamp
                  ).toLocaleString()}
                </p>
              )}

            </header>


            {vitalsLoading && (
              <p>Loading patient vitals...</p>
            )}

            {vitalsError && (
              <p role="alert">{vitalsError}</p>
            )}


            {/* VITAL-SIGN CARDS */}

            <section className="vitals-grid">

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

              <VitalCard
                title="Fall Detection"
                value="--"
                unit=""
                icon="🏃"
                status="Not connected"
              />

            </section>


            {/* HISTORICAL MEASUREMENTS */}

            <VitalsChart vitals={vitalsHistory} />

          </>
        )}

      </main>
    </div>
  );
}

export default CaregiverDashboard;
