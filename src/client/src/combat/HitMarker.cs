using Godot;

namespace DarkAges.Combat
{
    /// <summary>
    /// [CLIENT_AGENT] Crosshair hit marker
    /// </summary>
    public partial class HitMarker : TextureRect
    {
        [Export] public Color NormalColor = new(1, 1, 1, 0.8f);
        [Export] public Color CriticalColor = new(1, 0.5f, 0, 1);
        [Export] public float DisplayTime = 0.3f;
        
        private Timer? timer;

        public override void _Ready()
        {
            timer = new Timer();
            timer.WaitTime = DisplayTime;
            timer.OneShot = true;
            timer.Timeout += () => Hide();
            AddChild(timer);
            
            Hide();
        }

        public void ShowHit(bool isCritical)
        {
            Modulate = isCritical ? CriticalColor : NormalColor;
            Scale = isCritical ? new Vector2(1.5f, 1.5f) : Vector2.One;
            Show();
            timer?.Start();
        }
    }
}
