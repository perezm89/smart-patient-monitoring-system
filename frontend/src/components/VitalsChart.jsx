
import {
  ResponsiveContainer,
  LineChart,
  Line,
  XAxis,
  YAxis,
  CartesianGrid,
  Tooltip,
  Legend
} from 'recharts';

function VitalsChart({ vitals = [] }) {

  const chartData = vitals.map((record) => ({
    time: new Date(record.timestamp).toLocaleString([], {
      month: 'numeric',
      day: 'numeric',
      hour: '2-digit',
      minute: '2-digit'
    }),
    heartRate: record.heartRateBpm,
    spo2: record.spo2Percent,
    temperature: record.skinTemperatureC
  }));

  return (
    <section className="chart-container">
      <div className="chart-header">
        <h2>Vital Signs History</h2>
        <span className="demo-label">Recorded Data</span>
      </div>

      {chartData.length === 0 ? (
        <p>No patient vitals recorded yet.</p>
      ) : (
        <div className="chart-wrapper">
          <ResponsiveContainer width="100%" height="100%">
            <LineChart data={chartData}>
              <CartesianGrid
                stroke="#26343d"
                strokeDasharray="3 3"
              />

              <XAxis
                dataKey="time"
                stroke="#94a3b8"
              />

              <YAxis
                yAxisId="heart"
                domain={['auto', 'auto']}
                stroke="#fb7185"
              />

              <YAxis
                yAxisId="oxygen"
                orientation="right"
                domain={[0, 100]}
                stroke="#60a5fa"
              />

              <Tooltip
                contentStyle={{
                  backgroundColor: '#152028',
                  border: '1px solid #26343d',
                  borderRadius: '8px'
                }}
              />

              <Legend />

              <Line
                yAxisId="heart"
                type="monotone"
                dataKey="heartRate"
                name="Heart Rate (BPM)"
                stroke="#fb7185"
                strokeWidth={2}
                connectNulls={false}
              />

              <Line
                yAxisId="oxygen"
                type="monotone"
                dataKey="spo2"
                name="SpO₂ (%)"
                stroke="#60a5fa"
                strokeWidth={2}
                connectNulls={false}
              />
            </LineChart>
          </ResponsiveContainer>
        </div>
      )}

      <p className="chart-note">
        Historical measurements retrieved from MongoDB Atlas.
        Temperature trend visualization coming next.
      </p>
    </section>
  );
}

export default VitalsChart;
