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
            
            // Connect to network signals if NetworkManager available
            if (NetworkManager.Instance != null)
            {
                NetworkManager.Instance.InventorySyncReceived += OnInvSync;
                NetworkManager.Instance.InventoryUpdateReceived += OnInvUpdate;
                GD.Print("[InventoryPanel] Network connected");
            }
            
            // Populate with demo items (fallback)
            PopulateDemoItems();
            
            GD.Print($"[InventoryPanel] Initialized with {SlotCount} slots");
        }
        
        public override void _ExitTree()
        {
            if (NetworkManager.Instance != null)
            {
                NetworkManager.Instance.InventorySyncReceived -= OnInvSync;
                NetworkManager.Instance.InventoryUpdateReceived -= OnInvUpdate;
            }
        }
        
        private void SetupUI()
        {
            // Main container
            var vbox = new VBoxContainer();
            vbox.SetAnchorsPreset(LayoutPreset.FullRect);
            vbox.SizeFlagsHorizontal = SizeFlags.ExpandFill;
            vbox.SizeFlagsVertical = SizeFlags.ExpandFill;
            AddChild(vbox);
            
            // Header
            var header = new Label
            {
                Text = "Inventory",
                HorizontalAlignment = HorizontalAlignment.Center,
                ThemeTypeVariation = "HeaderLarge",
                CustomMinimumSize = new Vector2(0, 30)
            };
            vbox.AddChild(header);
            
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
            
            // Close button at bottom
            var closeBtn = new Button
            {
                Text = "Close (I)",
                CustomMinimumSize = new Vector2(0, 30),
                SizeFlagsHorizontal = SizeFlags.ExpandFill
            };
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
        
        // Network signal handlers
        private void OnInvSync(float gold, int[] itemIds, int[] quantities)
        {
            GD.Print($"[InventoryPanel] Sync: gold={gold}, slots={itemIds?.Length ?? 0}");
            
            // Clear all slots
            for (int i = 0; i < SlotCount; i++)
                if (_slots.ContainsKey(i)) _slots[i].ClearItem();
            
            // Populate from sync
            if (itemIds != null && quantities != null)
            {
                for (int i = 0; i < itemIds.Length && i < SlotCount; i++)
                {
                    if (itemIds[i] > 0 && quantities[i] > 0 && _slots.ContainsKey(i))
                    {
                        string name = ItemIdToName((uint)itemIds[i]);
                        var color = ItemIdToColor((uint)itemIds[i]);
                        _slots[i].SetItem(name, quantities[i], color);
                    }
                }
            }
        }
        
        private void OnInvUpdate(int slotIdx, int itemId, int qty)
        {
            if (slotIdx < 0 || slotIdx >= SlotCount || !_slots.ContainsKey(slotIdx)) return;
            
            if (itemId > 0 && qty > 0)
            {
                _slots[slotIdx].SetItem(ItemIdToName((uint)itemId), qty, ItemIdToColor((uint)itemId));
            }
            else
            {
                _slots[slotIdx].ClearItem();
            }
            GD.Print($"[InventoryPanel] Update: slot={slotIdx} item={itemId} qty={qty}");
        }

        public string ItemIdToName(uint id)
        {
            var m = new Dictionary<uint, string> {
                {1,"HP"}, {2,"MP"}, {3,"Sword"}, {4,"Armor"}, {5,"Wood"}, {6,"Stone"},
                {7,"Gold"}, {8,"Bread"}, {9,"Arrow"}, {10,"Scroll"}
            };
            return m.TryGetValue(id, out var n) ? n : $"Item{id}";
        }

        public Color ItemIdToColor(uint id)
        {
            var c = new Dictionary<uint, Color> {
                {1,new Color(0.8f,0.2f,0.2f)}, {2,new Color(0.2f,0.4f,0.9f)},
                {3,new Color(0.6f,0.6f,0.7f)}, {4,new Color(0.6f,0.6f,0.7f)},
                {7,new Color(1f,0.84f,0f)}
            };
            return c.TryGetValue(id, out var cl) ? cl : new Color(0.9f,0.9f,0.7f);
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
                // Background panel
                _background = new Panel();
                _background.SetAnchorsPreset(LayoutPreset.FullRect);
                _container.AddChild(_background);
                
                // Style the background
                var style = new StyleBoxFlat
                {
                    BgColor = new Color(0.15f, 0.15f, 0.15f, 0.9f),
                    BorderColor = new Color(0.3f, 0.3f, 0.3f, 1.0f),
                    BorderWidthBottom = 2,
                    BorderWidthLeft = 2,
                    BorderWidthRight = 2,
                    BorderWidthTop = 2
                };
                _background.AddThemeStyleboxOverride("panel", style);
                
                // Item icon (colored square)
                _itemIcon = new ColorRect();
                _itemIcon.SetAnchorsPreset(LayoutPreset.Center);
                _itemIcon.CustomMinimumSize = new Vector2(32, 32);
                _itemIcon.Color = new Color(0, 0, 0, 0); // Invisible by default
                _itemIcon.Visible = false;
                _container.AddChild(_itemIcon);
                
                // Stack count label
                _stackLabel = new Label();
                _stackLabel.SetAnchorsPreset(LayoutPreset.BottomRight);
                _stackLabel.OffsetLeft = -25;
                _stackLabel.OffsetTop = -20;
                _stackLabel.OffsetRight = -2;
                _stackLabel.Set("theme_override_font_sizes/font_size", 12);
                _stackLabel.HorizontalAlignment = HorizontalAlignment.Right;
                _container.AddChild(_stackLabel);
                
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
