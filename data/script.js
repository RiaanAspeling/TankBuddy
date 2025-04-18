// Custom gauge class
class WaterLevelGauge {
    constructor(canvasId, options) {
        this.canvas = document.getElementById(canvasId);
        this.ctx = this.canvas.getContext('2d');
        this.valueDisplay = document.getElementById('water-level-value');
        
        // Default options
        this.options = {
            minValue: 0,
            maxValue: 100,
            width: this.canvas.width,
            height: this.canvas.height,
            borderWidth: 4,
            borderColor: '#333',
            backgroundColor: '#f5f5f5',
            waterColor: '',
            highlightColor: '',
            highlightThreshold: 0,
            majorTicks: [10, 20, 30, 40, 50, 60, 70, 80, 90],
            animationDuration: 3000,
            decimals: 2
        };
        
        // Override defaults with provided options
        if (options) {
            Object.assign(this.options, options);
        }
        
        this._value = 0;
        this.targetValue = 0;
        this.animationStart = null;
        this.animating = false;
    }
    
    get value() {
        return this._value;
    }
    
    set value(newValue) {
        // Constrain value to min/max range
        newValue = Math.max(this.options.minValue, Math.min(this.options.maxValue, newValue));
        
        if (this._value !== newValue) {
            this.targetValue = newValue;
            this.animationStart = null;
            
            if (!this.animating) {
                this.animating = true;
                requestAnimationFrame(this.animate.bind(this));
            }
        }
    }
    
    animate(timestamp) {
        if (!this.animationStart) this.animationStart = timestamp;
        const elapsed = timestamp - this.animationStart;
        
        // Animation progress (0 to 1)
        const progress = Math.min(elapsed / this.options.animationDuration, 1);
        
        // Elastic easing function
        const eased = this.elasticEasing(progress);
        
        // Calculate current value based on animation progress
        this._value = this._value + (this.targetValue - this._value) * eased;
        
        // Draw the gauge
        this.draw();
        
        // Continue animation if not finished
        if (progress < 1) {
            requestAnimationFrame(this.animate.bind(this));
        } else {
            this._value = this.targetValue;
            this.animating = false;
        }
    }
    
    elasticEasing(t) {
        return ((33 * t * t * t * t * t - 106 * t * t * t * t + 126 * t * t * t - 67 * t * t + 15 * t) + 1) / 2;
    }
    
    draw() {
        const ctx = this.ctx;
        const opt = this.options;
        const width = opt.width;
        const height = opt.height;
        
        // Clear canvas
        ctx.clearRect(0, 0, width, height);
        
        // Draw gauge background
        ctx.fillStyle = opt.backgroundColor;
        this.roundRect(ctx, opt.borderWidth, opt.borderWidth, 
                        width - 2 * opt.borderWidth, 
                        height - 2 * opt.borderWidth, 10, true, false);
        
        // Calculate water height based on value
        const valuePercent = (this._value - opt.minValue) / (opt.maxValue - opt.minValue);
        const waterHeight = (height - 2 * opt.borderWidth) * valuePercent;
        const waterY = height - opt.borderWidth - waterHeight;
        
        // Draw water
        const waterColor = this._value >= opt.highlightThreshold ? opt.highlightColor : opt.waterColor;
        ctx.fillStyle = waterColor;
        this.roundRect(ctx, opt.borderWidth, waterY, 
                        width - 2 * opt.borderWidth, waterHeight, 
                        [0, 0, 10, 10], true, false);
        
        // Draw ticks and values
        this.drawTicks();
        
        // Draw border
        ctx.strokeStyle = opt.borderColor;
        ctx.lineWidth = opt.borderWidth;
        this.roundRect(ctx, opt.borderWidth / 2, opt.borderWidth / 2, 
                        width - opt.borderWidth, height - opt.borderWidth, 
                        10, false, true);
        
        // Update value display
        this.valueDisplay.textContent = this._value.toFixed(opt.decimals) + '%';
    }
    
    drawTicks() {
        const ctx = this.ctx;
        const opt = this.options;
        const width = opt.width;
        const height = opt.height;
        const ticks = opt.majorTicks;
        
        ctx.fillStyle = '#333';
        ctx.font = '12px Arial';
        ctx.textAlign = 'right';
        
        for (let i = 0; i < ticks.length; i++) {
            const tick = ticks[i];
            const tickPercent = (tick - opt.minValue) / (opt.maxValue - opt.minValue);
            const tickY = height - opt.borderWidth - (height - 2 * opt.borderWidth) * tickPercent;
            
            // Draw tick line
            ctx.beginPath();
            ctx.moveTo(opt.borderWidth, tickY);
            ctx.lineTo(width / 3, tickY);
            ctx.strokeStyle = '#333';
            ctx.lineWidth = 1;
            ctx.stroke();
            
            // Draw tick value
            ctx.fillText(tick.toString(), width / 3 - 20, tickY - 10);
        }
    }
    
    roundRect(ctx, x, y, width, height, radius, fill, stroke) {
        if (typeof radius === 'number') {
            radius = {tl: radius, tr: radius, br: radius, bl: radius};
        } else if (Array.isArray(radius)) {
            if (radius.length === 1) {
                radius = {tl: radius[0], tr: radius[0], br: radius[0], bl: radius[0]};
            } else if (radius.length === 2) {
                radius = {tl: radius[0], tr: radius[0], br: radius[1], bl: radius[1]};
            } else if (radius.length === 4) {
                radius = {tl: radius[0], tr: radius[1], br: radius[2], bl: radius[3]};
            }
        } else {
            radius = {tl: 0, tr: 0, br: 0, bl: 0};
        }
        
        ctx.beginPath();
        ctx.moveTo(x + radius.tl, y);
        ctx.lineTo(x + width - radius.tr, y);
        ctx.quadraticCurveTo(x + width, y, x + width, y + radius.tr);
        ctx.lineTo(x + width, y + height - radius.br);
        ctx.quadraticCurveTo(x + width, y + height, x + width - radius.br, y + height);
        ctx.lineTo(x + radius.bl, y + height);
        ctx.quadraticCurveTo(x, y + height, x, y + height - radius.bl);
        ctx.lineTo(x, y + radius.tl);
        ctx.quadraticCurveTo(x, y, x + radius.tl, y);
        ctx.closePath();
        
        if (fill) {
            ctx.fill();
        }
        
        if (stroke) {
            ctx.stroke();
        }
    }
}

// Initialize the gauge
var gaugeWaterLevel = new WaterLevelGauge('gauge-waterlevel', {
    minValue: 0,
    maxValue: 100,
    decimals: 1,
    highlightThreshold: 50,
    highlightColor: 'rgba(8, 125, 235, 0.75)',
    waterColor: 'rgba(54, 5, 233, 0.8)'
});

// Function to get current readings on the web page when it loads
function getReadings() {
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
            var myObj = JSON.parse(this.responseText);
            console.log(myObj);
            gaugeWaterLevel.value = myObj.waterlevel;
        }
    };
    xhr.open("GET", "/readings", true);
    xhr.send();
}

// Get current sensor readings when the page loads
window.addEventListener('load', getReadings);

// Setup EventSource for real-time updates
if (!!window.EventSource) {
    var source = new EventSource('/events');
    
    source.addEventListener('open', function(e) {
        console.log("Events Connected");
    }, false);
    
    source.addEventListener('error', function(e) {
        if (e.target.readyState != EventSource.OPEN) {
            console.log("Events Disconnected");
        }
    }, false);
    
    source.addEventListener('message', function(e) {
        console.log("message", e.data);
    }, false);
    
    source.addEventListener('new_readings', function(e) {
        console.log("new_readings", e.data);
        var myObj = JSON.parse(e.data);
        console.log(myObj);
        gaugeWaterLevel.value = myObj.waterlevel;
    }, false);
}
