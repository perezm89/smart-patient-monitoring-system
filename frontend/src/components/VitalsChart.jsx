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

const demoData = [
  { time: '08:00', heartRate: 72, spo2: 98, temperature: 36.4 },
  { time: '09:00', heartRate: 76, spo2: 98, temperature: 36.5 },
  { time: '10:00', heartRate: 74, spo2: 97, temperature: 36.6 },
  { time: '11:00', heartRate: 80, spo2: 98, temperature: 36.5 },
  { time: '12:00', heartRate: 77, spo2: 99, temperature: 36.7 },
  { time: '13:00', heartRate: 75, spo2: 98, temperature: 36.6 }
];

function VitalsChart() {
  return (
    <section className="chart-container">
      <div className="chart-header">
        <h2>Vital Signs History</h2>
        <span className="demo-label">Demo Data</span>
      </div>

      <div className="chart-wrapper">
        <ResponsiveContainer width="100%" height="100%">
          <LineChart data={demoData}>
            <CartesianGrid stroke="#26343d" strokeDasharray="3 3" />

            <XAxis dataKey="time" stroke="#94a3b8" />

            <YAxis
              yAxisId="heart"
              domain={[40, 120]}
              stroke="#fb7185"
            />

            <YAxis
              yAxisId="oxygen"
              orientation="right"
              domain={[90, 100]}
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
            />

            <Line
              yAxisId="oxygen"
              type="monotone"
              dataKey="spo2"
              name="SpO₂ (%)"
              stroke="#60a5fa"
              strokeWidth={2}
            />
          </LineChart>
        </ResponsiveContainer>
      </div>

      <p className="chart-note">
        Temperature: separate trend visualization coming next.
      </p>
    </section>
  );
}

export default VitalsChart;