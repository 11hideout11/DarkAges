using Godot;
using System;
using System.Collections.Generic;
using DarkAges.Networking;

namespace DarkAges.Client.UI
{
    /// <summary>
    /// [CLIENT_AGENT] Inventory grid UI for demo
    /// Displays 24-slot inventory with item icons, stack counts, and tooltips
    /// </summary>
    public partial class InventoryPanel : Panel
    {
        [Export] public int SlotCount { get; set; } = 24;
        [Export] public int SlotsPerRow { get; set; } = 6;
        [Export] public Vector2 SlotSize { get; set; } = new Vector2(50, 50);
        [Export] public bool VisibleByDefault { get; set; } = false;
        
        private GridContainer _grid;
        private Label _titleLabel;
        private Dictionary<int, InventorySlotUI> _slots = new Dictionary<int, InventorySlotUI>();
        private bool _isVisible = false;
        
        // Sample items for demo
        private readonly string[] _sampleItemNames = {
            "Health Potion", "Mana Potion", "Iron Sword", "Leather Armor",
            "Wood", "Stone", "Gold Coin", "Bread", "Arrow", "Scroll"
        };
        
        public override void _Ready()
        {
            SetupUI();
            
            // Start hidden unless requested
            _isVisible = VisibleByDefault;
            Visible = _isVisible;
            
            // Populate with demo items
            PopulateDemoItems();
            
            GD.Print($"[InventoryPanel] Initialized with {SlotCount} slots");
        }
        
        private void SetupUI()
        {
            // Apply theme background to the panel itself
            var panelStyle = UITheme.CreatePanelStyle(cornerRadius: 6f, borderWidth: 2f);
            AddThemeStyleboxOverride("panel", panelStyle);
            
            // Main container
            var vbox = new VBoxContainer();
            vbox.SetAnchorsPreset(LayoutPreset.FullRect);
            vbox.SizeFlagsHorizontal = SizeFlags.ExpandFill;
            vbox.SizeFlagsVertical = SizeFlags.ExpandFill;
            AddChild(vbox);
            
            // Header with theme styling
            _titleLabel = new Label
            {
                Text = "Inventory",
                HorizontalAlignment = HorizontalAlignment.Center,
                CustomMinimumSize = new Vector2(0, 36)
            };
            _titleLabel.AddThemeFontSizeOverride("font_size", UITheme.FontSizeHeader);
            _titleLabel.Modulate = UITheme.AccentPrimary;
            vbox.AddChild(_titleLabel);
            
            // Scroll container for the grid
            var scroll = new ScrollContainer
            {
                SizeFlagsVertical = SizeFlags.ExpandFill,
                SizeFlagsHorizontal = SizeFlags.ExpandFill
            };
            vbox.AddChild(scroll);
            
            // Grid container
            _grid = new GridContainer
            {
                Columns = SlotsPerRow,
                SizeFlagsHorizontal = SizeFlags.ExpandFill
            };
            scroll.AddChild(_grid);
            
            // Create slots
            for (int i = 0; i < SlotCount; i++)
            {
                CreateSlot(i);
            }
            
            // Close button at bottom with theme styling
            var closeBtn = new Button
            {
                Text = "Close (I)",
                CustomMinimumSize = new Vector2(0, 36),
                SizeFlagsHorizontal = SizeFlags.ExpandFill
            };
            closeBtn.AddThemeFontSizeOverride("font_size", UITheme.FontSizeBody);
            
            var buttonStyles = UITheme.CreateButtonStyles(cornerRadius: 4f);
            closeBtn.AddThemeStyleboxOverride("normal", buttonStyles[0]);
            closeBtn.AddThemeStyleboxOverride("hover", buttonStyles[1]);
            closeBtn.AddThemeStyleboxOverride("pressed", buttonStyles[2]);
            closeBtn.AddThemeStyleboxOverride("disabled", buttonStyles[3]);
            
            closeBtn.Pressed += () => Toggle(false);
            vbox.AddChild(closeBtn);
        }
        
        private void CreateSlot(int index)
        {
            var slotContainer = new Control
            {
                CustomMinimumSize = SlotSize
            };
            _grid.AddChild(slotContainer);
            
            var slot = new InventorySlotUI(index, slotContainer);
            _slots[index] = slot;
            
            // Handle slot click
            slot.OnSlotClicked += OnSlotClicked;
        }
        
        private void PopulateDemoItems()
        {
            var random = new Random();
            
            // Add some sample items to random slots
            int itemsToAdd = 8 + random.Next(8); // 8-16 items
            
            for (int i = 0; i < itemsToAdd; i++)
            {
                int slotIndex = random.Next(SlotCount);
                string itemName = _sampleItemNames[random.Next(_sampleItemNames.Length)];
                int stackSize = random.Next(1, itemName.Contains("Coin") ? 99 : 5);
                
                // Generate a color based on item type
                Color itemColor = GetItemColor(itemName);
                
                _slots[slotIndex].SetItem(itemName, stackSize, itemColor);
            }
        }
        
        private Color GetItemColor(string itemName)
        {
            if (itemName.Contains("Potion"))
                return new Color(0.8f, 0.2f, 0.2f); // Red for potions
            if (itemName.Contains("Sword") || itemName.Contains("Armor"))
                return new Color(0.6f, 0.6f, 0.7f); // Gray for equipment
            if (itemName.Contains("Gold"))
                return new Color(1.0f, 0.84f, 0.0f); // Gold for currency
            if (itemName.Contains("Wood"))
                return new Color(0.55f, 0.35f, 0.2f); // Brown for wood
            if (itemName.Contains("Stone"))
                return new Color(0.5f, 0.5f, 0.5f); // Gray for stone
            if (itemName.Contains("Mana") || itemName.Contains("Scroll"))
                return new Color(0.2f, 0.4f, 0.9f); // Blue for magic
            if (itemName.Contains("Arrow"))
                return new Color(0.8f, 0.6f, 0.3f); // Wood/brown for arrows
            
            return new Color(0.9f, 0.9f, 0.7f); // Default cream
        }
        
        private void OnSlotClicked(int slotIndex)
        {
            var slot = _slots[slotIndex];
            if (slot.HasItem)
            {
                GD.Print($"[InventoryPanel] Clicked slot {slotIndex}: {slot.ItemName} x{slot.StackSize}");
            }
        }
        
        public override void _Input(InputEvent @event)
        {
            // Toggle with 'I' key
            if (@event is InputEventKey keyEvent && keyEvent.Pressed)
            {
                if (keyEvent.Keycode == Key.I)
                {
                    Toggle(!_isVisible);
                }
            }
        }
        
        public void Toggle(bool show)
        {
            _isVisible = show;
            Visible = show;
            
            GD.Print($"[InventoryPanel] Visibility: {show}");
        }
        
        /// <summary>
        /// Individual inventory slot UI
        /// </summary>
        private class InventorySlotUI
        {
            public int Index { get; }
            public bool HasItem { get; private set; } = false;
            public string ItemName { get; private set; } = "";
            public int StackSize { get; private set; } = 0;
            
            public event Action<int> OnSlotClicked;
            
            private Control _container;
            private Panel _background;
            private ColorRect _itemIcon;
            private Label _stackLabel;
            private PopupPanel _tooltip;
            
            public InventorySlotUI(int index, Control container)
            {
                Index = index;
                _container = container;
                SetupUI();
            }
            
            private void SetupUI()
            {
                // Background panel with theme styling
                _background = new Panel();
                _background.SetAnchorsPreset(LayoutPreset.FullRect);
                _container.AddChild(_background);
                
                // Use theme slot style
                var slotStyle = UITheme.CreateSlotStyle(cornerRadius: 3f);
                _background.AddThemeStyleboxOverride("panel", slotStyle);
                
                // Item icon (colored square)
                _itemIcon = new ColorRect();
                _itemIcon.SetAnchorsPreset(LayoutPreset.Center);
                _itemIcon.CustomMinimumSize = new Vector2(32, 32);
                _itemIcon.Color = new Color(0, 0, 0, 0); // Invisible by default
                _itemIcon.Visible = false;
                _container.AddChild(_itemIcon);
                
                // Stack count label with theme styling
                _stackLabel = new Label();
                _stackLabel.SetAnchorsPreset(LayoutPreset.BottomRight);
                _stackLabel.OffsetLeft = -25;
                _stackLabel.OffsetTop = -20;
                _stackLabel.OffsetRight = -2;
                _stackLabel.AddThemeFontSizeOverride("font_size", UITheme.FontSizeSmall);
                _stackLabel.Modulate = UITheme.TextPrimary;
                _stackLabel.HorizontalAlignment = HorizontalAlignment.Right;
                _container.AddChild(_stackLabel);
                
                // Hover effect - store for later use
                _background.MouseEntered += OnMouseEntered;
                _background.MouseExited += OnMouseExited;
                
                // Click handling
                _background.GuiInput += OnGuiInput;
                
                // Tooltip
                _tooltip = new PopupPanel();
                _tooltip.Visible = false;
                _tooltip.TransparentBg = false;
                
                var tooltipLabel = new Label();
                _tooltip.AddChild(tooltipLabel);
                
                _container.AddChild(_tooltip);
            }
            
            private void OnMouseEntered()
            {
                // Apply hover style
                var hoverStyle = UITheme.CreateHoverStyle(cornerRadius: 3f);
                _background.AddThemeStyleboxOverride("panel", hoverStyle);
            }
            
            private void OnMouseExited()
            {
                // Revert to normal slot style
                var normalStyle = UITheme.CreateSlotStyle(cornerRadius: 3f);
                _background.AddThemeStyleboxOverride("panel", normalStyle);
            }
            
            private void OnGuiInput(InputEvent @event)
            {
                if (@event is InputEventMouseButton mouseBtn && mouseBtn.Pressed)
                {
                    if (mouseBtn.ButtonIndex == MouseButton.Left)
                    {
                        OnSlotClicked?.Invoke(Index);
                    }
                }
            }
            
            public void SetItem(string name, int stackSize, Color color)
            {
                HasItem = true;
                ItemName = name;
                StackSize = stackSize;
                
                _itemIcon.Color = color;
                _itemIcon.Visible = true;
                
                _stackLabel.Text = stackSize > 1 ? stackSize.ToString() : "";
            }
            
            public void ClearItem()
            {
                HasItem = false;
                ItemName = "";
                StackSize = 0;
                
                _itemIcon.Color = new Color(0, 0, 0, 0);
                _itemIcon.Visible = false;
                _stackLabel.Text = "";
            }
        }
    }
}
