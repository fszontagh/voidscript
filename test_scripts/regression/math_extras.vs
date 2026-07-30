// Roadmap Tier 3: Math extras - atan2/hypot/sign/clamp/gcd/lcm/deg2rad/rad2deg.
printnl(hypot(3.0, 4.0));                  // 5.000000
printnl(sign(-5), " ", sign(0), " ", sign(9));  // -1 0 1
printnl(clamp(15.0, 0.0, 10.0));           // 10.000000
printnl(clamp(-3.0, 0.0, 10.0));           // 0.000000
printnl(gcd(48, 36), " ", lcm(4, 6));      // 12 12
printnl(deg2rad(180.0));                    // 3.141593
printnl(rad2deg(3.141592653589793));        // 180.000000
printnl("done");
