function VitalCard({ title, value, unit, icon, status }) {
  return (
    <div className="vital-card">
      <div className="vital-card-header">
        <span className="vital-icon">{icon}</span>
        <span className="vital-title">{title}</span>
      </div>

      <div className="vital-value">
        {value} <span className="vital-unit">{unit}</span>
      </div>

      <div className="vital-status">
        <span className="status-dot"></span>
        {status}
      </div>
    </div>
  );
}

export default VitalCard;